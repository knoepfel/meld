#include "meld/core/store_counters.hpp"

#include "spdlog/spdlog.h"

namespace meld {

  void store_flag::flush_received(std::size_t const original_message_id)
  {
    flush_received_ = true;
    original_message_id_ = original_message_id;
  }

  bool store_flag::is_flush(level_id const* id) noexcept
  {
    if (id) {
      spdlog::info(" ===> Checking {}: Processed {}  Flush received {}",
                   id->to_string(),
                   processed_,
                   flush_received_);
    }
    return processed_ and flush_received_;
  }

  void store_flag::mark_as_processed() noexcept { processed_ = true; }

  unsigned int store_flag::original_message_id() const noexcept { return original_message_id_; }

  store_flag& detect_flush_flag::flag_for(level_id::hash_type const hash, flag_accessor& ca)
  {
    if (!flags_.find(ca, hash)) {
      flags_.emplace(ca, hash, std::make_unique<store_flag>());
    }
    return *ca->second;
  }

  bool detect_flush_flag::flag_for(level_id::hash_type const hash, const_flag_accessor& ca) const
  {
    return flags_.find(ca, hash);
  }

  void detect_flush_flag::erase_flag(const_flag_accessor& ca) { flags_.erase(ca); }

  // =====================================================================================

  void store_counter::set_flush_value(level_id const& id, std::size_t const original_message_id)
  {
    stop_after_ = id.back();
    original_message_id_ = original_message_id;
  }

  void store_counter::increment() noexcept { ++count_; }

  bool store_counter::is_flush(level_id const* id) noexcept
  {
    if (id) {
      spdlog::info(
        " ===> Checking {}: Count {}  Stop after {}", id->to_string(), count_, stop_after_);
    }
    auto stop_after = stop_after_.load();
    return count_.compare_exchange_strong(stop_after, -2u);
  }

  unsigned int store_counter::original_message_id() const noexcept { return original_message_id_; }

  store_counter& count_stores::counter_for(level_id::hash_type const hash, counter_accessor& ca)
  {
    if (!counters_.find(ca, hash)) {
      counters_.emplace(ca, hash, std::make_unique<store_counter>());
    }
    return *ca->second;
  }

  bool count_stores::counter_for(level_id::hash_type const hash, const_counter_accessor& ca) const
  {
    return counters_.find(ca, hash);
  }

  void count_stores::erase_counter(const_counter_accessor& ca) { counters_.erase(ca); }
}
