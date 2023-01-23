#include "meld/model/level_hierarchy.hpp"

#include "catch2/catch.hpp"

TEST_CASE("Level hierarchy", "[data model]")
{
  meld::level_hierarchy h;
  h.make_factory("block");
  h.make_factory("run", "subrun", "event");
  h.make_factory("run", "trigger record");
  h.make_factory("run", "trigger record"); // Adding another instance of the same order
                                           // should not expand the cached-order list.

  std::vector<std::vector<std::string>> const expected_orders{
    {"block"}, {"run", "subrun", "event"}, {"run", "trigger record"}};
  // CHECK(expected_orders == h.orders());
}
