#include "meld/core/store_counters.hpp"
#include "meld/model/level_counter.hpp"

#include "spdlog/spdlog.h"

namespace meld {

  void store_flag::flush_received(std::size_t const original_message_id)
  {
    flush_received_ = true;
    original_message_id_ = original_message_id;
  }

  bool store_flag::is_flush(level_id const* /*id*/) noexcept
  {
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

  void store_counter::set_flush_value(product_store_const_ptr const& store,
                                      std::size_t const original_message_id)
  {
    if (not store->contains_product("[flush]")) {
      return;
    }

#ifdef __cpp_lib_atomic_shared_ptr
    flush_counts_ = store->get_product<flush_counts_ptr>("[flush]");
#else
    atomic_store(&flush_counts_, store->get_product<flush_counts_ptr>("[flush]"));
#endif
    original_message_id_ = original_message_id;
  }

  void store_counter::increment(level_id::hash_type const level_hash) { ++counts_[level_hash]; }

  bool store_counter::is_flush()
  {
    if (!ready_to_flush_) {
      return false;
    }

#ifdef __cpp_lib_atomic_shared_ptr
    auto flush_counts = flush_counts_.load();
#else
    auto flush_counts = atomic_load(&flush_counts_);
#endif
    if (not flush_counts) {
      return false;
    }

    // The 'counts_' data member can be empty if the flush_counts member has been filled
    // but none of the children stores have been processed.
    if (counts_.empty() and !flush_counts->empty()) {
      return false;
    }

    for (auto const& [level_hash, count] : counts_) {
      auto maybe_count = flush_counts->count_for(level_hash);
      if (!maybe_count or count != *maybe_count) {
        return false;
      }
    }

    // Flush only once!
    return ready_to_flush_.exchange(false);
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
