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

TEST_CASE("Products available to children", "[data model]")
{
  // auto gen1 = make_product_store();
  // gen1->add_product("grandpa", 65);
  // auto gen2 = gen1->make_child(1);
  // gen2->add_product("dad", 35);
  // CHECK(gen2->get_product<int>("grandpa") == 65);
  // CHECK(gen2->get_product<int>("dad") == 35);

  // auto gen3 = gen2->make_child(2);
  // gen3->add_product("son", 10);
  // CHECK(gen3->get_product<int>("grandpa") == 65);
  // CHECK(gen3->get_product<int>("dad") == 35);
  // CHECK(gen3->get_product<int>("son") == 10);
}

TEST_CASE("Product store derivation", "[data model]")
{
  SECTION("Only one store")
  {
    auto store = make_product_store();
    auto stores = std::make_tuple(store);
    CHECK(store == more_derived(store, store));
    CHECK(store == most_derived_store(stores));
  }

  auto root = make_product_store();
  auto trunk = root->make_child(1);
  SECTION("Compare different generations")
  {
    CHECK(trunk == more_derived(root, trunk));
    CHECK(trunk == more_derived(trunk, root));
  }
  SECTION("Compare siblings (right is always favored)")
  {
    auto bole = root->make_child(2);
    CHECK(bole == more_derived(trunk, bole));
    CHECK(trunk == more_derived(bole, trunk));
  }

  auto limb = trunk->make_child(2);
  auto branch = limb->make_child(3);
  auto twig = branch->make_child(4);
  auto leaf = twig->make_child(5);

  auto order_a = std::make_tuple(root, trunk, limb, branch, twig, leaf);
  auto order_b = std::make_tuple(leaf, twig, branch, limb, trunk, root);
  auto order_c = std::make_tuple(twig, leaf, limb, branch, root, trunk);
  CHECK(leaf == most_derived_store(order_a));
  CHECK(leaf == most_derived_store(order_b));
  CHECK(leaf == most_derived_store(order_c));
}
