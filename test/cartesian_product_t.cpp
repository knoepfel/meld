#include "meld/utilities/cartesian_product.hpp"

#include "catch2/catch.hpp"

#include <tuple>
#include <utility>
#include <vector>

TEST_CASE("Tuple Cartesian product", "[utilities]")
{
  auto a = 'a';
  auto b = 'b';
  auto c = 'c';
  auto d = 'd';
  auto e = 'e';

  std::vector<std::pair<char, char>> actual;
  decltype(actual) const expected{{a, d}, {a, e}, {b, d}, {b, e}, {c, d}, {c, e}};
  meld::cartesian_product(
    std::tie(a, b, c), std::tie(d, e), [&actual](auto l, auto r) { actual.emplace_back(l, r); });
  CHECK(actual == expected);
}
