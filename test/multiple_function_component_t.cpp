#include "meld/core/make_component.hpp"
#include "meld/core/product_store.hpp"

#include "catch2/catch.hpp"

#include <algorithm>
#include <vector>

using namespace meld;

namespace {
  auto
  square_numbers(std::vector<unsigned> const& numbers)
  {
    std::vector<unsigned> result(size(numbers));
    transform(begin(numbers), end(numbers), begin(result), [](unsigned i) { return i * i; });
    return result;
  }

  auto
  sum_numbers(std::vector<unsigned> const& squared_numbers)
  {
    return accumulate(begin(squared_numbers), end(squared_numbers), 0u);
  }

  double
  sqrt_sum_numbers(unsigned summed_numbers, unsigned offset)
  {
    return std::sqrt(static_cast<double>(summed_numbers + offset));
  }

  struct A {
    auto
    sqrt_sum(unsigned summed_numbers, unsigned offset) const
    {
      return sqrt_sum_numbers(summed_numbers, offset);
    }
  };
}

TEST_CASE("Call multiple functions", "[programming model]")
{
  auto store = make_product_store();
  store->add_product("numbers", std::vector<unsigned>{0, 1, 2, 3, 4});
  store->add_product("offset", 6u);
  framework_graph graph{framework_graph::run_once, store};

  SECTION("One component, all free functions")
  {
    auto component = make_component();
    component.declare_transform("square_numbers", square_numbers)
      .input("numbers")
      .output("squared_numbers");
    component.declare_transform("sum_numbers", sum_numbers)
      .input("squared_numbers")
      .output("summed_numbers");
    component.declare_transform("sqrt_sum_numbers", sqrt_sum_numbers)
      .input("summed_numbers", "offset")
      .output("result");
    graph.merge(component.release_functions());
  }

  SECTION("Multiple components, each with one free function")
  {
    auto a = make_component();
    a.declare_transform("square_numbers", square_numbers)
      .input("numbers")
      .output("squared_numbers");

    auto b = make_component();
    b.declare_transform("sum_numbers", sum_numbers)
      .input("squared_numbers")
      .output("summed_numbers");

    auto c = make_component();
    c.declare_transform("sqrt_sum_numbers", sqrt_sum_numbers)
      .input("summed_numbers", "offset")
      .output("result");

    graph.merge(a.release_functions());
    graph.merge(b.release_functions());
    graph.merge(c.release_functions());
  }

  SECTION("Multiple components, mixed free and member functions")
  {
    auto a = make_component();
    a.declare_transform("square_numbers", square_numbers)
      .input("numbers")
      .output("squared_numbers");

    auto b = make_component();
    b.declare_transform("sum_numbers", sum_numbers)
      .input("squared_numbers")
      .output("summed_numbers");

    auto c = make_component<A>();
    c.declare_transform("sqrt_sum_numbers", &A::sqrt_sum)
      .input("summed_numbers", "offset")
      .output("result");

    graph.merge(a.release_functions());
    graph.merge(b.release_functions());
    graph.merge(c.release_functions());
  }

  // The following is invoked for *each* section above
  graph.finalize_and_run();

  using numbers_t = std::vector<unsigned>;
  numbers_t const expected_squared_numbers{0, 1, 4, 9, 16};
  CHECK(store->get_product<numbers_t>("squared_numbers") == expected_squared_numbers);
  CHECK(store->get_product<unsigned>("summed_numbers") == 30u);
  CHECK(store->get_product<double>("result") == 6.);
}
