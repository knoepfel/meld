#include "meld/core/make_component.hpp"
#include "meld/core/product_store.hpp"
#include "meld/utilities/debug.hpp"

#include "catch2/catch.hpp"

#include <string>
#include <tuple>
#include <vector>

using namespace meld;

namespace {
  auto
  no_framework(int num, double temp, std::string const& name)
  {
    return std::make_tuple(num, temp, name);
  }

  auto
  no_framework_all_refs(int const& num, double const& temp, std::string const& name)
  {
    return std::make_tuple(num, temp, name);
  }

  auto
  no_framework_all_ptrs(int const* num, double const* temp, std::string const* name)
  {
    return std::make_tuple(*num, *temp, *name);
  }

  auto
  one_framework_arg(handle<int> num, double temp, std::string const& name)
  {
    return std::make_tuple(*num, temp, name);
  }

  auto
  all_framework_args(handle<int> const num,
                     handle<double> const temp,
                     handle<std::string> const name)
  {
    return std::make_tuple(*num, *temp, *name);
  }
}

TEST_CASE("Call non-framework functions", "[programming model]")
{
  using result_t = std::tuple<int, double, std::string>;
  std::vector<std::string> const product_names{"number", "temperature", "name"};
  std::vector<std::string> const result{"result"};

  auto component = make_component();
  SECTION("No framework")
  {
    component.declare_transform("no_framework", no_framework).input(product_names).output(result);
  }
  SECTION("No framework, all references")
  {
    component.declare_transform("no_framework_all_refs", no_framework_all_refs)
      .input(product_names)
      .output(result);
  }
  SECTION("No framework, all pointers")
  {
    component.declare_transform("no_framework_all_ptrs", no_framework_all_ptrs)
      .input(product_names)
      .output(result);
  }
  SECTION("One framework argument")
  {
    component.declare_transform("one_framework_arg", one_framework_arg)
      .input(product_names)
      .output(result);
  }
  SECTION("All framework arguments")
  {
    component.declare_transform("all_framework_args", all_framework_args)
      .input(product_names)
      .output(result);
  }

  // The following is invoked for *each* section above
  auto store = make_product_store();
  store->add_product("number", 3);
  store->add_product("temperature", 98.5);
  store->add_product("name", std::string{"John"});

  framework_graph graph{framework_graph::run_once, store};
  graph.merge(component.release_transforms(), component.release_reductions());
  graph.finalize_and_run();

  auto const expected = std::make_tuple(3, 98.5, "John");
  CHECK(store->get_product<result_t>("result") == expected);
}
