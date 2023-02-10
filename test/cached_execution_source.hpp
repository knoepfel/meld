#ifndef test_cached_execution_source_hpp
#define test_cached_execution_source_hpp

// ===================================================================
// This source creates:
//
//  1 run
//    2 subruns per run
//      5 events per subrun
// ===================================================================

#include "meld/core/cached_product_stores.hpp"
#include "meld/model/level_id.hpp"

#include <cassert>

namespace test {
  inline constexpr std::size_t n_runs{1};
  inline constexpr std::size_t n_subruns{2u};
  inline constexpr std::size_t n_events{5000u};

  class cached_execution_source {
  public:
    // Uncopyable due to iterator
    cached_execution_source(cached_execution_source const&) = delete;
    cached_execution_source& operator=(cached_execution_source const&) = delete;

    cached_execution_source()
    {
      using namespace meld;

      auto const job_id = level_id::base_ptr();
      levels_.push_back(job_id);
      for (std::size_t i = 0; i != n_runs; ++i) {
        auto const run_id = job_id->make_child(i, "run");
        levels_.push_back(run_id);
        for (std::size_t j = 0; j != n_subruns; ++j) {
          auto const subrun_id = run_id->make_child(j, "subrun");
          levels_.push_back(subrun_id);
          for (std::size_t k = 0; k != n_events; ++k) {
            auto event_id = subrun_id->make_child(k, "event");
            levels_.push_back(event_id);
          }
        }
      }
      current_ = begin(levels_);
      assert(size(levels_) == 1 + n_runs * (1 + n_subruns * (1 + n_events)));
    }

    meld::product_store_ptr next()
    {
      if (current_ == end(levels_)) {
        return nullptr;
      }
      auto const id = *current_++;

      auto store = cached_stores_.get_store(id);
      if (id->level_name() == "run") {
        store->add_product<int>("number", 2 * store->id()->number());
      }
      if (id->level_name() == "subrun") {
        store->add_product<int>("another", 3 * store->id()->number());
      }
      if (id->level_name() == "event") {
        store->add_product<int>("still", 4 * store->id()->number());
      }
      return store;
    }

  private:
    std::vector<meld::level_id_ptr> levels_;
    meld::cached_product_stores cached_stores_{};
    decltype(levels_)::iterator current_;
  };
}

#endif /* test_cached_execution_source_hpp */
