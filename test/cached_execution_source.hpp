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
#include "meld/graph/transition.hpp"

namespace test {
  inline constexpr std::size_t n_runs{1};
  inline constexpr std::size_t n_subruns{2u};
  inline constexpr std::size_t n_events{5u};

  class cached_execution_source {
  public:
    // Uncopyable due to iterator
    cached_execution_source(cached_execution_source const&) = delete;
    cached_execution_source& operator=(cached_execution_source const&) = delete;

    cached_execution_source()
    {
      using namespace meld;

      meld::level_id const job_id{};
      transitions_.emplace_back(level_id{}, stage::process);
      for (std::size_t i = 0; i != n_runs; ++i) {
        auto const run_id = job_id.make_child(i);
        transitions_.emplace_back(run_id, stage::process);
        for (std::size_t j = 0; j != n_subruns; ++j) {
          auto const subrun_id = run_id.make_child(j);
          transitions_.emplace_back(subrun_id, stage::process);
          for (std::size_t k = 0; k != n_events; ++k) {
            auto event_id = subrun_id.make_child(k);
            transitions_.emplace_back(event_id, stage::process);
          }
        }
      }
      current_ = begin(transitions_);
    }

    meld::product_store_ptr next()
    {
      if (current_ == end(transitions_)) {
        return nullptr;
      }
      auto const& [id, stage] = *current_++;

      auto store = stores_.get_empty_store(id, stage);
      if (store->is_flush()) {
        return store;
      }
      if (store->id().depth() == 1ull) {
        store->add_product<int>("number", 2 * store->id().back());
      }
      if (store->id().depth() == 2ull) {
        store->add_product<int>("another", 3 * store->id().back());
      }
      if (store->id().depth() == 3ull) {
        store->add_product<int>("still", 4 * store->id().back());
      }
      return store;
    }

  private:
    std::vector<meld::transition> transitions_;
    meld::cached_product_stores stores_;
    decltype(transitions_)::iterator current_;
  };
}

#endif /* test_cached_execution_source_hpp */
