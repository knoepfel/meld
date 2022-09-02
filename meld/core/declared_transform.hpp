#ifndef meld_core_declared_transform_hpp
#define meld_core_declared_transform_hpp

#include "meld/core/fwd.h"
#include "meld/core/product_store.hpp"
#include "meld/utilities/sized_tuple.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <span>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class declared_transform {
  public:
    declared_transform(std::string name, std::size_t concurrency);

    virtual ~declared_transform();

    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    tbb::flow::receiver<product_store_ptr>& port(std::string const& product_name);
    virtual tbb::flow::sender<product_store_ptr>& sender() = 0;
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;
    virtual std::span<std::string const, std::dynamic_extent> output() const = 0;

  private:
    virtual tbb::flow::receiver<product_store_ptr>& port_for(std::string const& product_name) = 0;

    std::string name_;
    std::size_t concurrency_;
  };

  using declared_transform_ptr = std::unique_ptr<declared_transform>;
  using declared_transforms = std::map<std::string, declared_transform_ptr>;

  // Registering concrete transforms

  template <typename T, typename R, typename... Args>
  class incomplete_transform {
    static constexpr auto N = sizeof...(Args);
    template <std::size_t M>
    class complete_transform;
    class transform_requires_output;

  public:
    incomplete_transform(user_functions<T>& funcs,
                         std::string name,
                         tbb::flow::graph& g,
                         R (*f)(Args...)) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{f}
    {
    }

    template <typename FT>
    incomplete_transform(user_functions<T>& funcs, std::string name, tbb::flow::graph& g, FT f) :
      funcs_{funcs}, name_{move(name)}, graph_{g}, ft_{std::move(f)}
    {
    }

    // Icky?
    incomplete_transform&
    concurrency(std::size_t n)
    {
      concurrency_ = n;
      return *this;
    }

    auto
    input(std::array<std::string, N> input_keys)
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
    auto
    input(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      static_assert(N == sizeof...(Ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::array<std::string, N>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{tbb::flow::serial};
    tbb::flow::graph& graph_;
    std::function<R(Args...)> ft_;
  };

  template <typename T, typename R, typename... Args>
  template <std::size_t M>
  class incomplete_transform<T, R, Args...>::complete_transform : public declared_transform {
    std::size_t
    port_index_for(std::string const& product_name)
    {
      auto it = std::find(cbegin(input_), cend(input_), product_name);
      if (it == cend(input_)) {
        throw std::runtime_error("Product name " + product_name + " not valid for transform.");
      }
      return std::distance(cbegin(input_), it);
    }

    template <std::size_t I>
    tbb::flow::receiver<product_store_ptr>&
    receiver_for(std::size_t const index)
    {
      if constexpr (I < N) {
        if (I != index) {
          return receiver_for<I + 1ull>(index);
        }
        return input_port<I>(join_);
      }
      else {
        throw std::runtime_error("Should never get here");
      }
    }

    template <std::size_t... Is>
    R
    call(std::function<R(Args...)> ft, stores_t<N> const& stores, std::index_sequence<Is...>)
    {
      return std::invoke(
        ft,
        std::get<Is>(stores)->template get_handle<typename handle_for<Args>::value_type>(
          input_[Is])...);
    }

  public:
    complete_transform(std::string name,
                       std::size_t concurrency,
                       tbb::flow::graph& g,
                       std::function<R(Args...)>&& f,
                       std::array<std::string, N> input,
                       std::array<std::string, M> output) :
      declared_transform{move(name), concurrency},
      input_{move(input)},
      output_{move(output)},
      join_{g, type_for_t<ProductStoreHasher, Args>{}...},
      transform_{g, concurrency, [this, ft = std::move(f)](stores_t<N> const& stores) {
                   auto const& original_store = std::get<0>(stores); // FIXME!
                   if (original_store->is_flush()) {
                     return original_store;
                   }

                   auto store = most_derived_store(stores)->extend();
                   if constexpr (std::same_as<R, void>) {
                     call(ft, stores, std::index_sequence_for<Args...>{});
                   }
                   else {
                     auto result = call(ft, stores, std::index_sequence_for<Args...>{});
                     store->add_product(output_[0], result);
                   }
                   return store;
                 }}
    {
      if constexpr (N > 1ull) {
        make_edge(join_, transform_);
      }
      else {
        make_edge(join_.pass_through, transform_);
      }
    }

  private:
    tbb::flow::receiver<product_store_ptr>&
    port_for(std::string const& product_name) override
    {
      if constexpr (N > 1ull) {
        auto const index = port_index_for(product_name);
        return receiver_for<0ull>(index);
      }
      else {
        return join_.pass_through;
      }
    }

    tbb::flow::sender<product_store_ptr>&
    sender() override
    {
      return transform_;
    }

    std::span<std::string const, std::dynamic_extent>
    input() const override
    {
      return input_;
    }
    std::span<std::string const, std::dynamic_extent>
    output() const override
    {
      return output_;
    }

    std::array<std::string, N> input_;
    std::array<std::string, M> output_;
    join_or_none_t<N> join_;
    tbb::flow::function_node<stores_t<N>, product_store_ptr> transform_;
  };

  template <typename T, typename R, typename... Args>
  class incomplete_transform<T, R, Args...>::transform_requires_output {
  public:
    transform_requires_output(user_functions<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              tbb::flow::graph& g,
                              std::function<R(Args...)>&& f,
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
    void
    output(std::array<std::string, M> output_keys)
    {
      funcs_.add_transform(
        name_,
        std::make_unique<complete_transform<M>>(
          name_, concurrency_, graph_, move(ft_), move(input_keys_), move(output_keys)));
    }

    template <typename... Ts>
    void
    output(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      // FIXME: Eventually make a static_assert so that # of output arguments matches sizeof...(Ts)
      output(std::array<std::string, sizeof...(Ts)>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_;
    tbb::flow::graph& graph_;
    std::function<R(Args...)> ft_;
    std::array<std::string, N> input_keys_;
  };
}

#endif /* meld_core_declared_transform_hpp */
