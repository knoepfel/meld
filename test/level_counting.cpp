#include "meld/model/level_counter.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/level_id.hpp"
#include "meld/model/product_store.hpp"

#include "catch2/catch.hpp"

using namespace meld;

TEST_CASE("Counter with nothing processed", "[data model]")
{
  level_counter job_counter{};
  CHECK(job_counter.result().empty());
}

TEST_CASE("Counter one layer deep", "[data model]")
{
  level_counter job_counter{};
  for (std::size_t i = 0; i != 10; ++i) {
    job_counter.make_child("event");
  }
  CHECK(job_counter.result().count_for("event") == 10);
}

TEST_CASE("Counter multiple layers deep", "[data model]")
{
  constexpr std::size_t nruns{2ull};
  constexpr std::size_t nsubruns_per_run{3ull};
  constexpr std::size_t nevents_per_subrun{5ull};

  std::size_t processed_jobs{};
  std::size_t processed_runs{};
  std::size_t processed_subruns{};
  std::size_t processed_events{};

  level_hierarchy h;

  // Notice the wholesale capture by reference--generally a lazy way of doing things.
  auto check_all_processed = [&] {
    CHECK(h.count_for("job") == processed_jobs);
    CHECK(h.count_for("run") == processed_runs);
    CHECK(h.count_for("subrun") == processed_subruns);
    CHECK(h.count_for("event") == processed_events);
  };

  auto job_store = product_store::base();
  h.update(job_store->id());
  for (std::size_t i = 0; i != nruns; ++i) {
    auto run_store = job_store->make_child(i, "run");
    h.update(run_store->id());
    for (std::size_t j = 0; j != nsubruns_per_run; ++j) {
      auto subrun_store = run_store->make_child(j, "subrun");
      h.update(subrun_store->id());
      for (std::size_t k = 0; k != nevents_per_subrun; ++k) {
        auto event_store = subrun_store->make_child(k, "event");
        h.update(event_store->id());
        ++processed_events;
        auto results = h.complete(event_store->id());
        CHECK(results.empty());
        check_all_processed();
      }
      auto results = h.complete(subrun_store->id());
      ++processed_subruns;
      CHECK(results.count_for("event"));
      check_all_processed();
    }
    auto results = h.complete(run_store->id());
    ++processed_runs;
    CHECK(results.count_for("event") == nevents_per_subrun * nsubruns_per_run);
    CHECK(results.count_for("subrun") == nsubruns_per_run);
    check_all_processed();
  }
  auto results = h.complete(job_store->id());
  ++processed_jobs;

  CHECK(results.count_for("event") == nevents_per_subrun * nsubruns_per_run * nruns);
  CHECK(results.count_for("subrun") == nsubruns_per_run * nruns);
  CHECK(results.count_for("run") == nruns);

  check_all_processed();
}
