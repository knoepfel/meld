#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/configuration.hpp"
#include "meld/core/declared_filter.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/filter/result_collector.hpp"
#include "meld/core/glue.hpp"
#include "meld/core/graph_proxy.hpp"
#include "meld/core/message.hpp"
#include "meld/core/multiplexer.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"
#include "meld/utilities/usage.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/info.h"

#include <cassert>
#include <functional>
#include <map>
#include <queue>
#include <stack>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace meld {

  class level_sentry {
  public:
    level_sentry(level_hierarchy& hierarchy,
                 std::queue<product_store_ptr>& pending_stores,
                 product_store_ptr store);
    ~level_sentry();
    std::size_t depth() const noexcept;

  private:
    level_hierarchy& hierarchy_;
    std::queue<product_store_ptr>& pending_stores_;
    product_store_ptr store_;
    std::size_t depth_;
  };

  class framework_graph {
  public:
    explicit framework_graph(product_store_ptr store,
                             int max_parallelism = oneapi::tbb::info::default_concurrency());
    explicit framework_graph(std::function<product_store_ptr()> f,
                             int max_parallelism = oneapi::tbb::info::default_concurrency());
    ~framework_graph();

    void execute(std::string const& dot_file_name = {});

    graph_proxy<void_tag> proxy(configuration const& config)
    {
      return {config,
              graph_,
              filters_,
              monitors_,
              outputs_,
              reductions_,
              splitters_,
              transforms_,
              registration_errors_};
    }

    // Framework function registrations

    // N.B. declare_output() is not directly accessible through framework_graph.  Is this
    //      right?

    template <typename... Args>
    auto declare_filter(std::string name, bool (*f)(Args...))
    {
      return proxy().declare_filter(move(name), f);
    }
    auto declare_filter(auto f) { return declare_filter(function_name(f), f); }

    template <typename... Args>
    auto declare_monitor(std::string name, void (*f)(Args...))
    {
      return proxy().declare_monitor(move(name), f);
    }
    auto declare_monitor(auto f) { return declare_monitor(function_name(f), f); }

    template <typename R, typename... Args, typename... InitArgs>
    auto declare_reduction(std::string name, void (*f)(R&, Args...), InitArgs&&... init_args)
    {
      return proxy().declare_reduction(move(name), f, std::forward<InitArgs>(init_args)...);
    }
    template <typename R, typename... Args, typename... InitArgs>
    auto declare_reduction(void (*f)(R&, Args...), InitArgs&&... init_args)
    {
      return declare_reduction(function_name(f), f, std::forward<InitArgs>(init_args)...);
    }

    template <typename... Args>
    auto declare_splitter(std::string name, void (*f)(Args...))
    {
      return proxy().declare_splitter(move(name), f);
    }
    auto declare_splitter(auto f) { return declare_splitter(function_name(f), f); }

    template <typename R, typename... Args>
    auto declare_transform(std::string name, R (*f)(Args...))
    {
      return proxy().declare_transform(move(name), f);
    }
    auto declare_transform(auto f) { return declare_transform(function_name(f), f); }

    template <typename T, typename... Args>
    glue<T> make(Args&&... args)
    {
      return {graph_,
              filters_,
              monitors_,
              outputs_,
              reductions_,
              splitters_,
              transforms_,
              std::make_shared<T>(std::forward<Args>(args)...),
              registration_errors_};
    }

  private:
    void run();
    void finalize(std::string const& dot_file_name);

    void accept(product_store_ptr store);
    void drain();
    message send(product_store_ptr store);
    std::size_t original_message_id(product_store_ptr const& store);
    product_store_ptr pending_store();

    glue<void_tag> proxy()
    {
      return {graph_,
              filters_,
              monitors_,
              outputs_,
              reductions_,
              splitters_,
              transforms_,
              nullptr,
              registration_errors_};
    }

    usage graph_usage{};
    concurrency::max_allowed_parallelism parallelism_limit_;
    tbb::flow::graph graph_{};
    level_hierarchy hierarchy_{};
    declared_filters filters_{};
    declared_monitors monitors_{};
    declared_outputs outputs_{};
    declared_reductions reductions_{};
    declared_splitters splitters_{};
    declared_transforms transforms_{};
    std::vector<std::string> registration_errors_{};
    std::map<std::string, result_collector> filter_collectors_{};
    tbb::flow::input_node<message> src_;
    multiplexer multiplexer_;
    std::map<level_id_ptr, std::size_t> original_message_ids_;
    std::queue<product_store_ptr> pending_stores_;
    std::stack<level_sentry> levels_;
    std::size_t calls_{};
    bool shutdown_{false};
  };
}

#endif /* meld_core_framework_graph_hpp */
