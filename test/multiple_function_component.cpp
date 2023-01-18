#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"

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

  void verify_result(double result, double expected) { CHECK(result == expected); }
}

TEST_CASE("Call multiple functions", "[programming model]")
{
  auto store = make_product_store();
  store->add_product("numbers", std::vector<unsigned>{0, 1, 2, 3, 4});
  store->add_product("offset", 6u);
  framework_graph g{store};

  SECTION("All free functions")
  {
    g.declare_transform(square_numbers)
      .concurrency(unlimited)
      .input("numbers")
      .output("squared_numbers");
    g.declare_transform(sum_numbers)
      .concurrency(unlimited)
      .input("squared_numbers")
      .output("summed_numbers");
    g.declare_transform(sqrt_sum_numbers)
      .concurrency(unlimited)
      .input("summed_numbers", "offset")
      .output("result");
  }

  SECTION("Transforms, one from a component")
  {
    g.declare_transform(square_numbers)
      .concurrency(unlimited)
      .input("numbers")
      .output("squared_numbers");
    g.declare_transform(sum_numbers)
      .concurrency(unlimited)
      .input("squared_numbers")
      .output("summed_numbers");
    g.make<A>()
      .declare_transform(&A::sqrt_sum)
      .concurrency(unlimited)
      .input("summed_numbers", "offset")
      .output("result");
  }

  // The following is invoked for *each* section above
  g.declare_monitor(verify_result).input("result", use(6.));
  g.execute();
}
