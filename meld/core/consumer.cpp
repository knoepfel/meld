#include "meld/core/consumer.hpp"

namespace meld {

  void consumer::store_counter::set_flush_value(level_id const& id,
                                                std::size_t const original_message_id)
  {
    stop_after_ = id.back();
    original_message_id_ = original_message_id;
  }

  void consumer::store_counter::increment() noexcept { ++count_; }

  bool consumer::store_counter::is_flush() noexcept
  {
    if (not processed_) {
      return false;
    }
    auto stop_after = stop_after_.load();
    return count_.compare_exchange_strong(stop_after, -2u);
  }

  void consumer::store_counter::mark_as_processed() noexcept { processed_ = true; }

  unsigned int consumer::store_counter::original_message_id() const noexcept
  {
    return original_message_id_;
  }

  consumer::consumer(std::vector<std::string> preceding_filters) :
    preceding_filters_{move(preceding_filters)}
  {
  }

  consumer::~consumer() = default;

  std::vector<std::string> const& consumer::filtered_by() const noexcept
  {
    return preceding_filters_;
  }
  std::size_t consumer::num_inputs() const { return input().size(); }

  tbb::flow::receiver<message>& consumer::port(std::string const& product_name)
  {
    return port_for(product_name);
  }

  consumer::store_counter& consumer::counter_for(level_id::hash_type const hash,
                                                 counter_accessor& ca)
  {
    if (!counters_.find(ca, hash)) {
      counters_.emplace(ca, hash, std::make_unique<store_counter>());
    }
    return *ca->second;
  }

  bool consumer::counter_for(level_id::hash_type const hash, const_counter_accessor& ca) const
  {
    return counters_.find(ca, hash);
  }

  void consumer::erase_counter(const_counter_accessor& ca) { counters_.erase(ca); }
}
