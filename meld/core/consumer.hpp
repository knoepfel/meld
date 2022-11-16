#ifndef meld_core_consumer_hpp
#define meld_core_consumer_hpp

#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/graph/transition.hpp"

#include "oneapi/tbb/concurrent_hash_map.h"
#include "oneapi/tbb/flow_graph.h"

#include <atomic>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace meld {
  class consumer {
    class store_counter {
    public:
      void set_flush_value(level_id const& id, std::size_t original_message_id);
      void increment() noexcept;
      bool is_flush() noexcept;
      void mark_as_processed() noexcept; // <= FIXME: Explain what this is for
      unsigned int original_message_id() const noexcept;

    private:
      std::atomic<bool> processed_{false};
      std::atomic<unsigned int> count_{};
      std::atomic<unsigned int> stop_after_{-1u};
      unsigned int
        original_message_id_{}; // Necessary for matching inputs to downstream join nodes.
    };

    using counters_t =
      tbb::concurrent_hash_map<level_id::hash_type, std::unique_ptr<store_counter>>;

  public:
    explicit consumer(std::vector<std::string> preceding_filters);

    virtual ~consumer();

    std::vector<std::string> const& filtered_by() const noexcept;
    std::size_t num_inputs() const;

    tbb::flow::receiver<message>& port(std::string const& product_name);
    virtual std::span<std::string const, std::dynamic_extent> input() const = 0;

  protected:
    using counter_accessor = counters_t::accessor;
    using const_counter_accessor = counters_t::const_accessor;
    store_counter& counter_for(level_id::hash_type hash, counter_accessor& ca);
    bool counter_for(level_id::hash_type hash, const_counter_accessor& ca) const;
    void erase_counter(const_counter_accessor& ca);

  private:
    virtual tbb::flow::receiver<message>& port_for(std::string const& product_name) = 0;

    std::vector<std::string> preceding_filters_;
    counters_t counters_;
  };
}

#endif /* meld_core_consumer_hpp */
