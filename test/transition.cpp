#include "meld/model/level_counter.hpp"
#include "meld/model/level_id.hpp"

#include "catch2/catch.hpp"

using namespace meld;

TEST_CASE("Transition string literal", "[transition]")
{
  CHECK(""_id == level_id::base_ptr());
  auto id = "2"_id;
  CHECK(*id == *id_for({2}));
  CHECK(id->hash() != 0ull);
  CHECK(*"1:2:3:4"_id == *id_for({1, 2, 3, 4}));
}

TEST_CASE("Hash IDs", "[transition]") { CHECK(""_id->hash() == 0ull); }

TEST_CASE("Parent counter", "[transition]")
{
  level_counter counter;
  counter.record_parent(*""_id);
  CHECK(counter.value(*""_id) == 0);

  counter.record_parent(*"1"_id);
  counter.record_parent(*"2"_id);
  CHECK(counter.value(*""_id) == 2);

  counter.record_parent(*"1:2:3"_id);
  CHECK(counter.value(*"1"_id) == 0);
  CHECK(counter.value(*"1:2"_id) == 1);
}
