#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/core/component.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/message.hpp"
#include "meld/core/multiplexer.hpp"
#include "meld/core/product_store.hpp"

#include "oneapi/tbb/flow_graph.h"

#include <cassert>
#include <functional>
#include <map>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class framework_graph {
  public:
    struct run_once_t {};
    static constexpr run_once_t run_once{};

    explicit framework_graph(run_once_t, product_store_ptr store);

    template <typename FT>
    explicit framework_graph(FT ft) :
      src_{graph_,
           [this, user_function = std::move(ft)](tbb::flow_control& fc) mutable -> message {
             auto store = user_function();
             if (not store) {
               fc.stop();
               return {};
             }

             ++calls_;
             if (store->is_flush()) {
               // Original message ID no longer needed after this message.
               auto h = original_message_ids_.extract(store->id().parent());
               assert(h);
               std::size_t const original_message_id = h.mapped();
               return {store, calls_, original_message_id};
             }

             // debug("Inserting message ID for ", store->id(), ": ", calls_);
             original_message_ids_.try_emplace(store->id(), calls_);
             return {store, calls_};
           }},
      multiplexer_{graph_}
    {
    }

    void execute(std::string const& dot_file_name = {});

    // Framework function registrations
    template <typename R, typename... Args>
    auto declare_transform(std::string name, R (*f)(Args...))
    {
      return unbound_functions_.declare_transform(move(name), f);
    }

    template <typename R, typename... Args, typename... InitArgs>
    auto declare_reduction(std::string name, void (*f)(R&, Args...), InitArgs&&... init_args)
    {
      return unbound_functions_.declare_reduction(
        move(name), f, std::forward<InitArgs>(init_args)...);
    }

    template <typename... Args>
    auto declare_splitter(std::string name, void (*f)(Args...))
    {
      return unbound_functions_.declare_splitter(move(name), f);
    }

    template <typename T, typename... Args>
    auto make(Args&&... args)
    {
      return unbound_functions_.bind_to<T>(std::forward<Args>(args)...);
    }

  private:
    void run();
    void finalize(std::string const& dot_file_name);

    tbb::flow::graph graph_{};
    declared_transforms transforms_{};
    declared_reductions reductions_{};
    declared_outputs outputs_{};
    declared_splitters splitters_{};
    component<void_tag> unbound_functions_{graph_, transforms_, reductions_, outputs_, splitters_};
    tbb::flow::input_node<message> src_;
    multiplexer multiplexer_;
    std::map<level_id, std::size_t> original_message_ids_;
    std::size_t calls_{};
  };
}

#endif /* meld_core_framework_graph_hpp */
