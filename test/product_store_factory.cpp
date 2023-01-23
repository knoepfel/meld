#include "meld/model/level_hierarchy.hpp"
#include "meld/model/transition.hpp"

#include "catch2/catch.hpp"

TEST_CASE("Product store factory", "[data model]")
{
  using namespace meld;
  level_hierarchy h;
  auto factory = h.make_factory("run", "subrun", "event");
  auto run_store = factory.make("1"_id);
  auto subrun_store = factory.make("1:2"_id);
  auto event_store = factory.make("1:2:3"_id);
  REQUIRE_THROWS(factory.make("1:2:3:4"_id));

  auto extended_factory = factory.extend("trigger sequence");
  auto trigger_sequence_store = extended_factory.make("1:2:3:4"_id);
}
