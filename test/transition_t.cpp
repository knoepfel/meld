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

TEST_CASE("Parent counter", "[transition]")
{
  level_counter counter;
  counter.record_parent(""_id);
  CHECK(counter.value(""_id) == 0);

  counter.record_parent("1"_id);
  counter.record_parent("2"_id);
  CHECK(counter.value(""_id) == 2);

  counter.record_parent("1:2:3"_id);
  CHECK(counter.value("1"_id) == 0);
  CHECK(counter.value("1:2"_id) == 1);

  CHECK(counter.value_as_id(""_id) == "2"_id);
  CHECK(counter.value_as_id("1"_id) == "1:0"_id);
  CHECK(counter.value_as_id("1:2"_id) == "1:2:1"_id);
}

TEST_CASE("Transitions between the same levels", "[transitions]")
{
  level_counter c;
  {
    auto const id = ""_id;
    CHECK(transitions_between(id, id, c) == transitions{});
  }
  {
    auto const id = "1:2:3:4"_id;
    CHECK(transitions_between(id, id, c) == transitions{});
  }
}

TEST_CASE("One-level transitions", "[transitions]")
{
  level_counter c;
  CHECK(transitions_between(""_id, "2"_id, c) == transitions{{"2"_id, stage::setup}});
  CHECK(transitions_between("2"_id, ""_id, c) ==
        transitions{{"2"_id, stage::process}, {"1"_id, stage::flush}});

  CHECK(transitions_between("1"_id, "1:2"_id, c) == transitions{{"1:2"_id, stage::setup}});
  CHECK(transitions_between("1:2"_id, "1"_id, c) ==
        transitions{{"1:2"_id, stage::process}, {"1:1"_id, stage::flush}});

  CHECK(transitions_between("2"_id, "3"_id, c) ==
        transitions{{"2"_id, stage::process}, {"3"_id, stage::setup}});

  CHECK(transitions_between("1:2"_id, "1:3"_id, c) ==
        transitions{{"1:2"_id, stage::process}, {"1:3"_id, stage::setup}});
}

TEST_CASE("Multi-level transitions", "[transitions]")
{
  level_counter c;
  CHECK(transitions_between(""_id, "1:2:3"_id, c) ==
        transitions{{"1"_id, stage::setup}, {"1:2"_id, stage::setup}, {"1:2:3"_id, stage::setup}});
  CHECK(transitions_between("1:2:3"_id, ""_id, c) == transitions{{"1:2:3"_id, stage::process},
                                                                 {"1:2:1"_id, stage::flush},
                                                                 {"1:2"_id, stage::process},
                                                                 {"1:1"_id, stage::flush},
                                                                 {"1"_id, stage::process},
                                                                 {"1"_id, stage::flush}});
  level_counter c1;
  CHECK(transitions_between("1:2:3:4"_id, "1:4"_id, c1) ==
        transitions{{"1:2:3:4"_id, stage::process},
                    {"1:2:3:1"_id, stage::flush},
                    {"1:2:3"_id, stage::process},
                    {"1:2:1"_id, stage::flush},
                    {"1:2"_id, stage::process},
                    {"1:4"_id, stage::setup}});
}

TEST_CASE("Multiple transitions", "[transitions]")
{
  CHECK(transitions_for({}) ==
        transitions{{""_id, stage::setup}, {"0"_id, stage::flush}, {""_id, stage::process}});
  CHECK(transitions_for({"1"_id}) == transitions{{""_id, stage::setup},
                                                 {"1"_id, stage::setup},
                                                 {"1"_id, stage::process},
                                                 {"1"_id, stage::flush},
                                                 {""_id, stage::process}});
  CHECK(transitions_for({"1"_id, "1:2"_id, "4"_id}) == transitions{{""_id, stage::setup},
                                                                   {"1"_id, stage::setup},
                                                                   {"1:2"_id, stage::setup},
                                                                   {"1:2"_id, stage::process},
                                                                   {"1:1"_id, stage::flush},
                                                                   {"1"_id, stage::process},
                                                                   {"4"_id, stage::setup},
                                                                   {"4"_id, stage::process},
                                                                   {"2"_id, stage::flush},
                                                                   {""_id, stage::process}});
}
