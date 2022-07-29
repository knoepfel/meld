#include "meld/core/product_store.hpp"

#include "catch2/catch.hpp"

#include <string>
#include <vector>

using namespace meld;

TEST_CASE("Product store insertion", "[data model]")
{
  product_store store;
  constexpr int number = 4;
  std::vector many_numbers{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  store.add_product("number", number);
  store.add_product("numbers", many_numbers);

  CHECK_THROWS_WITH(store.get_product<double>("number"),
                    Catch::Contains("Cannot get product 'number' with type 'double'."));

  auto invalid_handle = store.get_handle<int>("wrong_key");
  CHECK(!invalid_handle);
  auto const matcher = Catch::Contains("No product exists with the key 'wrong_key'.");
  CHECK_THROWS_WITH(*invalid_handle, matcher);
  CHECK_THROWS_WITH(invalid_handle.operator->(), matcher);

  CHECK(store.get_product<int>("number") == number);

  auto h = store.get_handle<std::vector<int>>("numbers");
  REQUIRE(h);
  CHECK(*h == many_numbers);
  CHECK(store.get_product<std::vector<int>>("numbers") == many_numbers);
}
