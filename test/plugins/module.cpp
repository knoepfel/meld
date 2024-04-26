#include "meld/module.hpp"
#include "test/plugins/add.hpp"

#include <cassert>

using namespace meld;

// TODO: Option to select which algorithm to run via configuration?

DEFINE_MODULE(m)
{
  m.with(test::add, concurrency::unlimited).transform("i", "j").to("sum");
  m.with("verify", [](int actual) { assert(actual == 0); }, concurrency::unlimited).monitor("sum");
}
