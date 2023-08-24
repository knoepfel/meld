#include "meld/core/framework_graph.hpp"
#include "meld/model/level_hierarchy.hpp"
#include "meld/model/product_store.hpp"

#include "catch2/catch.hpp"

#include <algorithm>
#include <vector>

using namespace meld;
using namespace meld::concurrency;

namespace {
  auto square_numbers(std::vector<unsigned> const& numbers)
  {
    std::vector<unsigned> result(size(numbers));
    transform(begin(numbers), end(numbers), begin(result), [](unsigned i) { return i * i; });
    return result;
  }

  auto sum_numbers(std::vector<unsigned> const& squared_numbers)
  {
    std::vector<unsigned> const expected_squared_numbers{0, 1, 4, 9, 16};
    CHECK(squared_numbers == expected_squared_numbers);
    return accumulate(begin(squared_numbers), end(squared_numbers), 0u);
  }

  double sqrt_sum_numbers(unsigned summed_numbers, unsigned offset)
  {
    CHECK(summed_numbers == 30u);
    return std::sqrt(static_cast<double>(summed_numbers + offset));
  }

  struct A {
    auto sqrt_sum(unsigned summed_numbers, unsigned offset) const
    {
      return sqrt_sum_numbers(summed_numbers, offset);
    }
  };
}

TEST_CASE("Call multiple functions", "[programming model]")
{
  auto store = product_store::base();
  store->add_product("numbers", std::vector<unsigned>{0, 1, 2, 3, 4});
  store->add_product("offset", 6u);
  framework_graph g{store};

  SECTION("All free functions")
  {
    g.with(square_numbers).transform("numbers").to("squared_numbers").using_concurrency(unlimited);
    g.with(sum_numbers)
      .transform("squared_numbers")
      .to("summed_numbers")
      .using_concurrency(unlimited);
    g.with(sqrt_sum_numbers)
      .transform("summed_numbers", "offset")
      .to("result")
      .using_concurrency(unlimited);
  }

  SECTION("Transforms, one from a class")
  {
    g.with(square_numbers).transform("numbers").to("squared_numbers").using_concurrency(unlimited);
    g.with(sum_numbers)
      .transform("squared_numbers")
      .to("summed_numbers")
      .using_concurrency(unlimited);
    g.make<A>()
      .with(&A::sqrt_sum)
      .transform("summed_numbers", "offset")
      .to("result")
      .using_concurrency(unlimited);
  }

  // The following is invoked for *each* section above
  g.with("verify_result", [](double actual) { assert(actual == 6.); }).monitor("result");
  g.execute();
}
