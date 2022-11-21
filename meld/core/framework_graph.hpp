#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/core/component.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/filter/result_collector.hpp"
#include "meld/core/message.hpp"
#include "meld/core/multiplexer.hpp"
#include "meld/core/product_store.hpp"
#include "meld/utilities/usage.hpp"

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
    explicit framework_graph(std::function<product_store_ptr()> f);

    void execute(std::string const& dot_file_name = {});

    // Framework function registrations

    // N.B. declare_output() is not directly accessible through framework_graph.  Is this
    //      right?

    template <typename... Args>
    auto declare_filter(std::string name, bool (*f)(Args...))
    {
      return unbound_functions_.declare_filter(move(name), f);
    }

    template <typename... Args>
    auto declare_monitor(std::string name, void (*f)(Args...))
    {
      return unbound_functions_.declare_monitor(move(name), f);
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

    template <typename R, typename... Args>
    auto declare_transform(std::string name, R (*f)(Args...))
    {
      return unbound_functions_.declare_transform(move(name), f);
    }

    template <typename T, typename... Args>
    auto make(Args&&... args)
    {
      return unbound_functions_.bind_to<T>(std::forward<Args>(args)...);
    }

  private:
    void run();
    void finalize(std::string const& dot_file_name);

    product_store_ptr next_pending_store();

    usage graph_usage{};
    tbb::flow::graph graph_{};
    declared_filters filters_{};
    declared_monitors monitors_{};
    declared_outputs outputs_{};
    declared_reductions reductions_{};
    declared_splitters splitters_{};
    declared_transforms transforms_{};
    std::map<std::string, result_collector> filter_collectors_{};
    component<void_tag> unbound_functions_{
      graph_, filters_, monitors_, outputs_, reductions_, splitters_, transforms_};
    tbb::flow::input_node<message> src_;
    multiplexer multiplexer_;
    std::map<level_id, std::size_t> original_message_ids_;
    product_store_ptr store_;
    std::queue<transition> pending_transitions_;
    level_id last_id_{};
    std::map<level_id, std::size_t> flush_values_;
    std::size_t calls_{};
    bool shutdown_{false};
  };
}

#endif /* meld_core_framework_graph_hpp */
