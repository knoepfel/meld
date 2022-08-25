#ifndef meld_core_declared_reduction_hpp
#define meld_core_declared_reduction_hpp

#include "meld/core/fwd.h"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <concepts>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class declared_reduction {
  public:
    declared_reduction(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> input_keys,
                       std::vector<std::string> output_keys);

    virtual ~declared_reduction();

    void invoke(product_store const& store);
    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    std::vector<std::string> const& input() const noexcept;
    std::vector<std::string> const& output() const noexcept;

    bool reduction_complete(product_store& parent_store);

  private:
    virtual void invoke_(product_store const& store) = 0;
    virtual void commit_(product_store& store) = 0;

    void set_flush_value(level_id const& id);

    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> input_keys_;
    std::vector<std::string> output_keys_;
    struct map_entry {
      std::atomic<unsigned int> count{};
      std::atomic<unsigned int> stop_after{-1u};
    };
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<map_entry>> entries_;
  };

  using declared_reduction_ptr = std::unique_ptr<declared_reduction>;
  using declared_reductions = std::map<std::string, declared_reduction_ptr>;

  // Registering concrete reductions

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction {
    class complete_reduction;
    class reduction_requires_output;

  public:
    incomplete_reduction(user_functions<T>& funcs,
                         std::string name,
                         void (*f)(R&, Args...),
                         InitTuple initializer) :
      funcs_{funcs}, name_{move(name)}, ft_{f}, initializer_{std::move(initializer)}
    {
    }

    template <typename FT>
    incomplete_reduction(user_functions<T>& funcs, std::string name, FT f, InitTuple initializer) :
      funcs_{funcs}, name_{move(name)}, ft_{std::move(f)}, initializer_{std::move(initializer)}
    {
    }

    // Icky?
    incomplete_reduction&
    concurrency(std::size_t concurrency)
    {
      concurrency_ = concurrency;
      return *this;
    }

    // FIXME: Should pick a different parameter type
    auto input(std::vector<std::string> input_keys);

    template <typename... Ts>
    auto
    input(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      static_assert(sizeof...(Args) == sizeof...(Ts),
                    "The number of function parameters is not the same as the number of specified "
                    "input arguments.");
      return input(std::vector<std::string>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_{tbb::flow::serial};
    std::function<void(R&, Args...)> ft_;
    InitTuple initializer_;
  };

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction<T, R, InitTuple, Args...>::complete_reduction :
    public declared_reduction {
  public:
    complete_reduction(std::string name,
                       std::size_t concurrency,
                       std::function<void(R&, Args...)>&& f,
                       InitTuple initializer,
                       std::vector<std::string> input,
                       std::vector<std::string> output) :
      declared_reduction{move(name), concurrency, move(input), move(output)},
      ft_{move(f)},
      initializer_{std::move(initializer)}
    {
    }

  private:
    template <size_t... Is>
    std::unique_ptr<R>
    initialized_object(InitTuple&& tuple, std::index_sequence<Is...>) const
    {
      return std::unique_ptr<R>{
        new R{std::forward<std::tuple_element_t<Is, InitTuple>>(std::get<Is>(tuple))...}};
    }

    void
    call(R& result, product_store const& store) // FIXME: 'icky'
    {
      using types = std::tuple<handle_for<Args>...>;
      auto const handles = [ this, &store ]<std::size_t... Is, typename... Ts>(
        std::index_sequence<Is...>, std::tuple<handle<Ts>...>)
      {
        return std::make_tuple(store.get_handle<Ts>(input()[Is])...);
      }
      (std::index_sequence_for<Args...>{}, types{});
      return std::apply(ft_, std::tuple_cat(std::tie(result), handles));
    }

    void
    invoke_(product_store const& store) override
    {
      auto const& parent = store.parent();
      auto it = results_.find(parent->id());
      if (it == results_.end()) {
        it =
          results_
            .insert({parent->id(),
                     initialized_object(std::move(initializer_),
                                        std::make_index_sequence<std::tuple_size_v<InitTuple>>{})})
            .first;
      }
      call(*it->second, store);
    }

    void
    commit_(product_store& store) override
    {
      auto& result = results_.at(store.id());
      if constexpr (requires { result->send(); }) {
        store.add_product(output()[0], result->send());
      }
      else {
        store.add_product(output()[0], move(result));
      }
      // Reclaim some memory; it would be better to erase the entire
      // entry from the map, but that is not thread-safe.
      // N.B. Calling reset() is safe even if move(result) has been
      // called.
      result.reset();
    }

    std::function<void(R&, Args...)> ft_;
    InitTuple initializer_;
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<R>> results_;
  };

  template <typename T, typename R, typename InitTuple, typename... Args>
  auto
  incomplete_reduction<T, R, InitTuple, Args...>::input(std::vector<std::string> input_keys)
  {
    if constexpr (std::same_as<
                    R,
                    void>) { // FIXME: It is suspect to have a void return type for reductions.
      funcs_.add_reduction(
        name_,
        std::make_unique<complete_reduction>(
          name_, concurrency_, move(ft_), std::move(initializer_), move(input_keys), {}));
      return;
    }
    else {
      return reduction_requires_output{
        funcs_, move(name_), concurrency_, move(ft_), std::move(initializer_), move(input_keys)};
    }
  }

  template <typename T, typename R, typename InitTuple, typename... Args>
  class incomplete_reduction<T, R, InitTuple, Args...>::reduction_requires_output {
  public:
    reduction_requires_output(user_functions<T>& funcs,
                              std::string name,
                              std::size_t concurrency,
                              std::function<void(R&, Args...)>&& f,
                              InitTuple initializer,
                              std::vector<std::string> input_keys) :
      funcs_{funcs},
      name_{move(name)},
      concurrency_{concurrency},
      ft_{move(f)},
      initializer_{std::move(initializer)},
      input_keys_{move(input_keys)}
    {
    }

    void
    output(std::vector<std::string> output_keys)
    {
      funcs_.add_reduction(name_,
                           std::make_unique<complete_reduction>(name_,
                                                                concurrency_,
                                                                move(ft_),
                                                                std::move(initializer_),
                                                                move(input_keys_),
                                                                move(output_keys)));
    }

    template <typename... Ts>
    void
    output(Ts... ts)
    {
      static_assert(std::conjunction_v<std::is_convertible<Ts, std::string>...>);
      output(std::vector<std::string>{ts...});
    }

  private:
    user_functions<T>& funcs_;
    std::string name_;
    std::size_t concurrency_;
    std::function<void(R&, Args...)> ft_;
    InitTuple initializer_;
    std::vector<std::string> input_keys_;
  };
}

#endif /* meld_core_declared_reduction_hpp */
