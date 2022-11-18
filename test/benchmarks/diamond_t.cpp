#include "meld/core/framework_graph.hpp"
#include "meld/graph/transition.hpp"
#include "meld/source.hpp"
#include "test/benchmarks/diamond_source.hpp"

#include "catch2/catch.hpp"

#include <cassert>

using namespace meld;
using namespace meld::concurrency;
using namespace std::string_literals;

namespace {
  int last_index(level_id const& id) { return static_cast<int>(id.back()); }
  int plus_one(int i) noexcept { return i + 1; }
  int plus_101(int i) noexcept { return i + 101; }
  void verify_difference(int i, int j) noexcept { assert(j - i == 100); }
}

TEST_CASE("Diamond dependency", "[benchmarks]")
{
  framework_graph g{detail::create_next<test::diamond_source>()};

  g.declare_transform("a_creator", last_index).concurrency(unlimited).input("id").output("a");
  g.declare_transform("b_creator", plus_one).concurrency(unlimited).input("a").output("b");
  g.declare_transform("c_creator", plus_101).concurrency(unlimited).input("a").output("c");
  g.declare_monitor("d", verify_difference).concurrency(unlimited).input("b", "c");

  g.execute("diamond_t.gv");
}
