#include "meld/core/specified_label.hpp"

#include "catch2/catch.hpp"

using namespace meld;

TEST_CASE("Empty label", "[data model]")
{
  specified_label empty{};
  CHECK_THROWS(""_in_each);
  CHECK_THROWS(""_in_each(""));
}

TEST_CASE("Only name in label", "[data model]")
{
  specified_label label{"product"};
  CHECK(label == "product"_in_each);

  // Empty domain string is interpreted as a wildcard--i.e. any domain.
  CHECK(label == "product"_in_each(""));
}

TEST_CASE("Label with domain", "[data model]")
{
  specified_label label{"product", {"event"}};
  CHECK(label == "product"_in_each("event"));
}
