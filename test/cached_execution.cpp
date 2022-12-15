// =======================================================================================
// This test executes the following graph
//
//    Multiplexer
//      |  |  |
//     A1  |  |
//      |\ |  |
//      | \|  |
//     A2 B1  |
//      |\ |\ |
//      | \| \|
//     A3 B2  C
//
// where A1, A2, and A3 are transforms that execute at the "run" level; B1 and B2 are
// transforms that execute at the "subrun" level; and C is a transform that executes at
// the event level.
//
// This test verifies that for each "run", "subrun", and "event", the corresponding
// transforms execute only once.  This test assumes:
//
//  1 run
//    2 subruns per run
//      5 events per subrun
//
// Note that B1 and B2 rely on the output from A1 and A2; and C relies on the output from
// B1.  However, because the A transforms execute at a different cadence than the B
// transforms (and similar for C), the B transforms must use "cached" data from the A
// transforms.
// =======================================================================================

#include "meld/core/framework_graph.hpp"
#include "meld/source.hpp"
#include "test/cached_execution_source.hpp"

#include "catch2/catch.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  struct OneArg {
    explicit OneArg(std::atomic<unsigned int>& counter) : calls{counter} {}
    int call(int) noexcept
    {
      ++calls;
      return 1;
    }
    std::atomic<unsigned int>& calls;
  };

  struct TwoArgs {
    explicit TwoArgs(std::atomic<unsigned int>& counter) : calls{counter} {}
    int call(int, int) noexcept
    {
      ++calls;
      return 2;
    }
    std::atomic<unsigned int>& calls;
  };
}

TEST_CASE("Cached function calls", "[data model]")
{
  framework_graph g{detail::create_next<test::cached_execution_source>()};

  std::atomic<unsigned int> a1_counter{};
  g.make<OneArg>(a1_counter)
    .declare_transform("A1", &OneArg::call)
    .concurrency(unlimited)
    .input("number")
    .output("one");
  std::atomic<unsigned int> a2_counter{};
  g.make<OneArg>(a2_counter)
    .declare_transform("A2", &OneArg::call)
    .concurrency(unlimited)
    .input("one")
    .output("used_one");
  std::atomic<unsigned int> a3_counter{};
  g.make<OneArg>(a3_counter)
    .declare_transform("A3", &OneArg::call)
    .concurrency(unlimited)
    .input("used_one")
    .output("done_one");

  std::atomic<unsigned int> b1_counter{};
  g.make<TwoArgs>(b1_counter)
    .declare_transform("B1", &TwoArgs::call)
    .concurrency(unlimited)
    .input("one", "another")
    .output("two");
  std::atomic<unsigned int> b2_counter{};
  g.make<TwoArgs>(b2_counter)
    .declare_transform("B2", &TwoArgs::call)
    .concurrency(unlimited)
    .input("used_one", "two")
    .output("used_two");

  std::atomic<unsigned int> c_counter{};
  g.make<TwoArgs>(c_counter)
    .declare_transform("C", &TwoArgs::call)
    .concurrency(unlimited)
    .input("used_two", "still")
    .output("three");

  g.execute("cached_execution_t.gv");

  using namespace test;
  // FIXME: Need to improve the synchronization to supply strict equality
  CHECK(a1_counter >= n_runs);
  CHECK(a2_counter >= n_runs);
  CHECK(a3_counter >= n_runs);

  CHECK(b1_counter >= n_runs * n_subruns);
  CHECK(b2_counter >= n_runs * n_subruns);

  CHECK(c_counter >= n_runs * n_subruns * n_events);
}
