#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"

#include <array>
#include <string>
#include <tuple>

using namespace meld;
using namespace std::string_literals;

namespace {
  auto no_framework(int num, double temp, std::string const& name)
  {
    return std::make_tuple(num, temp, name);
  }

  auto no_framework_all_refs(int const& num, double const& temp, std::string const& name)
  {
    return std::make_tuple(num, temp, name);
  }

  auto no_framework_all_ptrs(int const* num, double const* temp, std::string const* name)
  {
    return std::make_tuple(*num, *temp, *name);
  }

  auto one_framework_arg(handle<int> num, double temp, std::string const& name)
  {
    return std::make_tuple(*num, temp, name);
  }

  auto all_framework_args(handle<int> const num,
                          handle<double> const temp,
                          handle<std::string> const name)
  {
    return std::make_tuple(*num, *temp, *name);
  }

  void verify_results(std::tuple<int, double, std::string> const& result)
  {
    auto const expected = std::make_tuple(3, 98.5, "John");
    CHECK(result == expected);
  }

}

TEST_CASE("Call non-framework functions", "[programming model]")
{
  std::array const product_names{"number"s, "temperature"s, "name"s};
  std::array const result{"result"s};

  auto store = make_product_store();
  store->add_product("number", 3);
  store->add_product("temperature", 98.5);
  store->add_product("name", std::string{"John"});

  framework_graph g{framework_graph::run_once, store};
  SECTION("No framework")
  {
    g.make_component()
      .declare_transform("no_framework", no_framework)
      .input(product_names)
      .output(result);
  }
  SECTION("No framework, all references")
  {
    g.make_component()
      .declare_transform("no_framework_all_refs", no_framework_all_refs)
      .input(product_names)
      .output(result);
  }
  SECTION("No framework, all pointers")
  {
    g.make_component()
      .declare_transform("no_framework_all_ptrs", no_framework_all_ptrs)
      .input(product_names)
      .output(result);
  }
  SECTION("One framework argument")
  {
    g.make_component()
      .declare_transform("one_framework_arg", one_framework_arg)
      .input(product_names)
      .output(result);
  }
  SECTION("All framework arguments")
  {
    g.make_component()
      .declare_transform("all_framework_args", all_framework_args)
      .input(product_names)
      .output(result);
  }

  // The following is invoked for *each* section above
  g.make_component().declare_transform("verify_results", verify_results).input("result");

  g.execute();
}
