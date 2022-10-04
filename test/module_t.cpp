#include "meld/module.hpp"
#include "test/add.hpp"

using namespace meld::concurrency;

DEFINE_MODULE(m)
{
  m.declare_transform("add", test::add).concurrency(unlimited).input("i", "j").output("sum");
}
