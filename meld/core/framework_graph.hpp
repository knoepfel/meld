#ifndef meld_core_framework_graph_hpp
#define meld_core_framework_graph_hpp

#include "meld/configuration.hpp"
#include "meld/core/cached_product_stores.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/end_of_message.hpp"
#include "meld/core/filter.hpp"
#include "meld/core/glue.hpp"
#include "meld/core/graph_proxy.hpp"
#include "meld/core/message.hpp"
#include "meld/core/message_sender.hpp"
#include "meld/core/multiplexer.hpp"
#include "meld/core/node_catalog.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"
#include "meld/source.hpp"
#include "meld/utilities/resource_usage.hpp"

#include "oneapi/tbb/flow_graph.h"
#include "oneapi/tbb/info.h"

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
    level_sentry(flush_counters& counters, message_sender& sender, product_store_ptr store);
    ~level_sentry();
    std::size_t depth() const noexcept;

  private:
    flush_counters& counters_;
    message_sender& sender_;
    product_store_ptr store_;
    std::size_t depth_;
  };

  class framework_graph {
  public:
    explicit framework_graph(product_store_ptr store,
                             int max_parallelism = oneapi::tbb::info::default_concurrency());
    explicit framework_graph(std::function<product_store_ptr()> f,
                             int max_parallelism = oneapi::tbb::info::default_concurrency());
    explicit framework_graph(detail::next_store_t f,
                             int max_parallelism = oneapi::tbb::info::default_concurrency());
    ~framework_graph();

    void execute(std::string const& dot_prefix = {});

    std::size_t execution_counts(std::string const& node_name) const;
    std::size_t product_counts(std::string const& node_name) const;

    graph_proxy<void_tag> proxy(configuration const& config)
    {
      return {config, graph_, nodes_, registration_errors_};
    }

    // Framework function registrations

    // N.B. declare_output() is not directly accessible through framework_graph.  Is this
    //      right?

    auto with(std::string name, auto f, concurrency c = concurrency::serial)
    {
      return proxy().with(std::move(name), f, c);
    }
    auto with(auto f, concurrency c = concurrency::serial) { return with(function_name(f), f, c); }
    template <typename T>
    auto with(auto predicate, auto unfold, concurrency c = concurrency::serial)
    {
      return unfold_proxy<T>().declare_unfold(predicate, unfold, c);
    }

    template <typename T, typename... Args>
    glue<T> make(Args&&... args)
    {
      return {
        graph_, nodes_, std::make_shared<T>(std::forward<Args>(args)...), registration_errors_};
    }

  private:
    void run();
    void finalize(std::string const& dot_file_prefix);
    void post_data_graph(std::string const& dot_file_prefix);

    product_store_ptr accept(product_store_ptr store);
    void drain();
    std::size_t original_message_id(product_store_ptr const& store);

    glue<void_tag> proxy() { return {graph_, nodes_, nullptr, registration_errors_}; }

    template <typename T>
    splitter_glue<T> unfold_proxy()
    {
      return {graph_, nodes_, registration_errors_};
    }

    resource_usage graph_resource_usage_{};
    concurrency::max_allowed_parallelism parallelism_limit_;
    tbb::flow::graph graph_{};
    level_hierarchy hierarchy_{};
    node_catalog nodes_{};
    cached_product_stores stores_{};
    std::vector<std::string> registration_errors_{};
    std::map<std::string, filter> filters_{};
    tbb::flow::input_node<message> src_;
    multiplexer multiplexer_;
    std::stack<end_of_message_ptr> eoms_;
    message_sender sender_{hierarchy_, multiplexer_, eoms_};
    std::queue<product_store_ptr> pending_stores_;
    flush_counters counters_;
    std::stack<level_sentry> levels_;
    bool shutdown_{false};
  };
}

#endif /* meld_core_framework_graph_hpp */
