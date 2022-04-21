#include "meld/core/transition.hpp"

#include "catch2/catch.hpp"

using namespace meld;

TEST_CASE("Transition string literal", "[transition]")
{
  using meld::id_t;
  CHECK(""_id == id_t{});
  CHECK("2"_id == id_t{2});
  CHECK("1:2:3:4"_id == id_t{1, 2, 3, 4});
}

TEST_CASE("Transitions between the same levels", "[transitions]")
{
  CHECK(transitions_between({}, {}) == transitions{});

  auto const id = "1:2:3:4"_id;
  CHECK(transitions_between(id, id) == transitions{});
}

TEST_CASE("One-level transitions", "[transitions]")
{
  CHECK(transitions_between({}, "2"_id) == transitions{{"2"_id, stage::setup}});
  CHECK(transitions_between("2"_id, {}) == transitions{{"2"_id, stage::process}});

  CHECK(transitions_between("1"_id, "1:2"_id) == transitions{{"1:2"_id, stage::setup}});
  CHECK(transitions_between("1:2"_id, "1"_id) == transitions{{"1:2"_id, stage::process}});

  CHECK(transitions_between("1:2"_id, "1:3"_id) ==
        transitions{{"1:2"_id, stage::process}, {"1:3"_id, stage::setup}});
}

TEST_CASE("Multi-level transitions", "[transitions]")
{
  CHECK(transitions_between({}, "1:2:3"_id) ==
        transitions{{"1"_id, stage::setup}, {"1:2"_id, stage::setup}, {"1:2:3"_id, stage::setup}});
  CHECK(transitions_between("1:2:3"_id, {}) == transitions{
                                                 {"1:2:3"_id, stage::process},
                                                 {"1:2"_id, stage::process},
                                                 {"1"_id, stage::process},
                                               });
  CHECK(transitions_between("1:2:3:4"_id, "1:4"_id) == transitions{{"1:2:3:4"_id, stage::process},
                                                                   {"1:2:3"_id, stage::process},
                                                                   {"1:2"_id, stage::process},
                                                                   {"1:4"_id, stage::setup}});
}
