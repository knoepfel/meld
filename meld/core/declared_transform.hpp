#ifndef meld_core_declared_transform_hpp
#define meld_core_declared_transform_hpp

#include "meld/core/concurrency.hpp"
#include "meld/core/fwd.hpp"
#include "meld/core/handle.hpp"
#include "meld/core/message.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

namespace meld {

  class declared_transform {
  public:
    declared_transform(std::string name, std::size_t concurrency);

    virtual ~declared_transform();

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual tbb::flow::sender<message>& sender() = 0;
    virtual tbb::flow::sender<message>& to_output() = 0;
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;

  private:
    virtual tbb::flow::receiver<message>& port_for(std::string const& product_name) = 0;

    std::string name_;
    std::size_t concurrency_;
  };

  using declared_transform_ptr = std::unique_ptr<declared_transform>;
  using declared_transforms = std::map<std::string, declared_transform_ptr>;

  // Registering concrete transforms

  template <typename T, typename R, typename... Args>
  class incomplete_transform {
    static constexpr auto N = sizeof...(Args);
    using function_t = std::function<R(Args...)>;
    template <std::size_t M>
    class complete_transform;
    class transform_requires_output;

  public:
    incomplete_transform(component<T>& funcs, std::string name, tbb::flow::graph& g, function_t f) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{move(f)}
    {
    }

    // Icky?
    incomplete_transform& concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    auto input(std::array<std::string, N> input_keys)
    {
      if constexpr (std::same_as<R, void>) {
        funcs_.add_transform(
          name_,
          std::make_unique<complete_transform<0ull>>(name_,
                                                     concurrency_,
                                                     graph_,
                                                     move(ft_),
                                                     move(input_keys),
                                                     std::array<std::string, 0ull>{}));
        return;
      }
      else {
        return transform_requires_output{
          funcs_, move(name_), concurrency_, graph_, move(ft_), move(input_keys)};
      }
    }

    template <typename... Ts>
    auto input(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      static_assert(N == sizeof...(Ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::array<std::string, N>{ts...});
    }

  private:
    component<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{concurrency::serial};
    tbb::flow::graph& graph_;
    function_t ft_;
  };

  template <typename T, typename R, typename... Args>
  template <std::size_t M>
  class incomplete_transform<T, R, Args...>::complete_transform : public declared_transform {
  public:
    complete_transform(std::string name,
                       std::size_t concurrency,
                       tbb::flow::graph& g,
                       function_t&& f,
                       std::array<std::string, N> input,
                       std::array<std::string, M> output) :
      declared_transform{move(name), concurrency},
      input_{move(input)},
      output_{move(output)},
      join_{g, type_for_t<MessageHasher, Args>{}...},
      transform_{
        g, concurrency, [this, ft = std::move(f)](messages_t<N> const& messages, auto& output) {
          auto const& msg = most_derived(messages);
          auto const& [store, message_id] = std::tie(msg.store, msg.id);
          auto& [stay_in_graph, to_output] = output;
          if (store->is_flush()) {
            stay_in_graph.try_put(msg);
            return;
          }

          if (typename decltype(stores_)::const_accessor a; stores_.find(a, store->id())) {
            stay_in_graph.try_put({a->second, message_id});
            return;
          }

          typename decltype(stores_)::accessor a;
          bool const new_insert = stores_.insert(a, store->id());
          if (!new_insert) {
            stay_in_graph.try_put({a->second, message_id});
            return;
          }

          if constexpr (std::same_as<R, void>) {
            call(ft, messages, std::index_sequence_for<Args...>{});
            a->second = {};
          }
          else {
            auto result = call(ft, messages, std::index_sequence_for<Args...>{});
            auto new_store = make_product_store(store->id(), this->name());
            new_store->add_product(output_[0], result);
            a->second = new_store;
          }

          message const new_msg{a->second, message_id};
          stay_in_graph.try_put(new_msg);
          to_output.try_put(new_msg);
        }}
    {
      make_edge(join_, transform_);
    }

  private:
    tbb::flow::receiver<message>& port_for(std::string const& product_name) override
    {
      return receiver_for<N>(join_, input_, product_name);
    }

    tbb::flow::sender<message>& sender() override { return output_port<0>(transform_); }
    tbb::flow::sender<message>& to_output() override { return output_port<1>(transform_); }
    std::span<std::string const, std::dynamic_extent> input() const override { return input_; }
    std::span<std::string const, std::dynamic_extent> output() const override { return output_; }

    template <std::size_t... Is>
    R call(function_t const& ft, messages_t<N> const& messages, std::index_sequence<Is...>)
    {
      return std::invoke(ft, get_handle_for<Is, N, Args>(messages, input_)...);
    }

    std::array<std::string, N> input_;
    std::array<std::string, M> output_;
    join_or_none_t<N> join_;
    tbb::flow::multifunction_node<messages_t<N>, messages_t<2u>> transform_;
    tbb::concurrent_hash_map<level_id, product_store_ptr> stores_;
  };

  template <typename T, typename R, typename... Args>
  class incomplete_transform<T, R, Args...>::transform_requires_output {
  public:
    transform_requires_output(component<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              tbb::flow::graph& g,
                              function_t&& f,
                              std::array<std::string, N> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      concurrency_{concurrency},
      graph_{g},
      ft_{move(f)},
      input_keys_{move(input_keys)}
    {
    }

    template <std::size_t M>
    void output(std::array<std::string, M> output_keys)
    {
      funcs_.add_transform(
        name_,
        std::make_unique<complete_transform<M>>(
          name_, concurrency_, graph_, move(ft_), move(input_keys_), move(output_keys)));
    }

    template <typename... Ts>
    void output(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      // FIXME: Eventually make a static_assert so that # of output arguments matches sizeof...(Ts)
      output(std::array<std::string, sizeof...(Ts)>{ts...});
    }

  private:
    component<T>& funcs_;
    std::string name_;
    std::size_t concurrency_;
    tbb::flow::graph& graph_;
    function_t ft_;
    std::array<std::string, N> input_keys_;
  };
}

#endif /* meld_core_declared_transform_hpp */
