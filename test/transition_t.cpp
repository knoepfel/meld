#include "meld/core/transition.hpp"
#include "test/transition_specs.hpp"

#include "catch2/catch.hpp"

using namespace meld;
using namespace meld::test;

TEST_CASE("Transition string literal", "[transition]")
{
  CHECK(""_id == level_id{});
  CHECK("2"_id == level_id{2});
  CHECK("1:2:3:4"_id == level_id{1, 2, 3, 4});
}

TEST_CASE("Hash IDs", "[transition]") { CHECK(""_id.hash() == 0ull); }

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
  CHECK(transitions_between(""_id, "2"_id, c) == transitions{setup("2")});
  CHECK(transitions_between("2"_id, ""_id, c) == transitions{flush("2:0"), process("2")});

  CHECK(transitions_between("1"_id, "1:2"_id, c) == transitions{setup("1:2")});
  CHECK(transitions_between("1:2"_id, "1"_id, c) == transitions{flush("1:2:0"), process("1:2")});

  CHECK(transitions_between("2"_id, "3"_id, c) ==
        transitions{flush("2:0"), process("2"), setup("3")});

  CHECK(transitions_between("1:2"_id, "1:3"_id, c) ==
        transitions{flush("1:2:0"), process("1:2"), setup("1:3")});
}

TEST_CASE("Multi-level transitions", "[transitions]")
{
  level_counter c;
  CHECK(transitions_between(""_id, "1:2:3"_id, c) ==
        transitions{setup("1"), setup("1:2"), setup("1:2:3")});
  CHECK(transitions_between("1:2:3"_id, ""_id, c) == transitions{flush("1:2:3:0"),
                                                                 process("1:2:3"),
                                                                 flush("1:2:1"),
                                                                 process("1:2"),
                                                                 flush("1:1"),
                                                                 process("1")});
  level_counter c1;
  CHECK(transitions_between("1:2:3:4"_id, "1:4"_id, c1) == transitions{flush("1:2:3:4:0"),
                                                                       process("1:2:3:4"),
                                                                       flush("1:2:3:1"),
                                                                       process("1:2:3"),
                                                                       flush("1:2:1"),
                                                                       process("1:2"),
                                                                       setup("1:4")});
}

TEST_CASE("Multiple transitions", "[transitions]")
{
  CHECK(transitions_for({}) == transitions{setup(""), flush("0"), process("")});
  CHECK(transitions_for({"1"_id}) ==
        transitions{setup(""), setup("1"), flush("1:0"), process("1"), flush("1"), process("")});
  CHECK(transitions_for({"1"_id, "1:2"_id, "4"_id}) == transitions{setup(""),
                                                                   setup("1"),
                                                                   setup("1:2"),
                                                                   flush("1:2:0"),
                                                                   process("1:2"),
                                                                   flush("1:1"),
                                                                   process("1"),
                                                                   setup("4"),
                                                                   flush("4:0"),
                                                                   process("4"),
                                                                   flush("2"),
                                                                   process("")});
}
