#include "meld/core/product_store.hpp"

#include "catch2/catch.hpp"

#include <concepts>
#include <string>
#include <vector>

using namespace meld;

namespace {
  struct Composer {
    std::string name;
  };
}

TEST_CASE("Handle type conversions (compile-time checks)", "[data model]")
{
  static_assert(std::same_as<handle_for<int>, handle<int>>);
  static_assert(std::same_as<handle_for<int const>, handle<int>>);
  static_assert(std::same_as<handle_for<int const&>, handle<int>>);
  static_assert(std::same_as<handle_for<int const*>, handle<int>>);
}

TEST_CASE("Handle type conversions (run-time checks)", "[data model]")
{
  handle<double> empty;
  CHECK(not empty);

  product<int> const number{3};
  handle const h{number};
  CHECK(handle<int const>{number} == h);

  int const& num_ref = h;
  int const* num_ptr = h;
  CHECK(static_cast<bool>(h));
  CHECK(num_ref == 3);
  CHECK(*num_ptr == 3);

  product<Composer> const composer{{"Elgar"}};
  CHECK(handle { composer } -> name == "Elgar");
}
