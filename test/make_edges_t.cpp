#include "meld/graph/make_edges.hpp"

#include "catch2/catch.hpp"

#include <tuple>

using namespace meld;

TEST_CASE("Make edges", "[utilities]")
{
  auto a = 'a';
  auto b = 'b';
  auto c = 'c';
  auto d = 'd';
  auto e = 'e';

  std::vector<std::pair<char, char>> actual;
  decltype(actual) const expected{{a, b}, {a, c}, {a, d}, {b, e}, {c, e}, {d, e}};
  auto check_pairs = [&actual](auto l, auto r) { actual.emplace_back(l, r); };
  nodes_using(check_pairs, a)->nodes(b, c, d)->nodes(e);
  CHECK(actual == expected);
}
