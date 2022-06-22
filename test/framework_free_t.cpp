#include "meld/core/function_deducer.hpp"
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
  product_store store;
  store.add_product("number", 3);
  store.add_product("temperature", 98.5);
  store.add_product("name", std::string{"John"});

  std::vector<std::string> const product_names{"number", "temperature", "name"};

  auto const expected = std::make_tuple(3, 98.5, "John");
  CHECK(function_deducer(no_framework, product_names).call(store) == expected);
  CHECK(function_deducer(no_framework_all_refs, product_names).call(store) == expected);
  CHECK(function_deducer(no_framework_all_ptrs, product_names).call(store) == expected);
  CHECK(function_deducer(one_framework_arg, product_names).call(store) == expected);
  CHECK(function_deducer(all_framework_args, product_names).call(store) == expected);
}
