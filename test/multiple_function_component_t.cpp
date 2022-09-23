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

  void verify_result(double result) { CHECK(result == 6.); }
}

TEST_CASE("Call multiple functions", "[programming model]")
{
  auto store = make_product_store();
  store->add_product("numbers", std::vector<unsigned>{0, 1, 2, 3, 4});
  store->add_product("offset", 6u);
  framework_graph g{framework_graph::run_once, store};

  SECTION("One component, all free functions")
  {
    auto component = g.make_component();
    component.declare_transform("square_numbers", square_numbers)
      .concurrency(unlimited)
      .input("numbers")
      .output("squared_numbers");
    component.declare_transform("sum_numbers", sum_numbers)
      .concurrency(unlimited)
      .input("squared_numbers")
      .output("summed_numbers");
    component.declare_transform("sqrt_sum_numbers", sqrt_sum_numbers)
      .concurrency(unlimited)
      .input("summed_numbers", "offset")
      .output("result");
  }

  SECTION("Multiple components, each with one free function")
  {
    g.make_component()
      .declare_transform("square_numbers", square_numbers)
      .concurrency(unlimited)
      .input("numbers")
      .output("squared_numbers");
    g.make_component()
      .declare_transform("sum_numbers", sum_numbers)
      .concurrency(unlimited)
      .input("squared_numbers")
      .output("summed_numbers");
    g.make_component()
      .declare_transform("sqrt_sum_numbers", sqrt_sum_numbers)
      .concurrency(unlimited)
      .input("summed_numbers", "offset")
      .output("result");
  }

  SECTION("Multiple components, mixed free and member functions")
  {
    g.make_component()
      .declare_transform("square_numbers", square_numbers)
      .concurrency(unlimited)
      .input("numbers")
      .output("squared_numbers");
    g.make_component()
      .declare_transform("sum_numbers", sum_numbers)
      .concurrency(unlimited)
      .input("squared_numbers")
      .output("summed_numbers");
    g.make_component<A>()
      .declare_transform("sqrt_sum_numbers", &A::sqrt_sum)
      .concurrency(unlimited)
      .input("summed_numbers", "offset")
      .output("result");
  }

  // The following is invoked for *each* section above
  g.make_component().declare_transform("verify_result", verify_result).input("result");
  g.execute();
}
