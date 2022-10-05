#include "meld/module.hpp"
#include "test/plugins/add.hpp"

using namespace meld::concurrency;

// TODO: Option to select which algorithm to run via configuration?

DEFINE_MODULE(m)
{
  m.declare_transform("add", test::add).concurrency(unlimited).input("i", "j").output("sum");
  m.declare_transform("verify_zero", test::verify_zero)
    .concurrency(unlimited)
    .input("sum")
    .output(""); // FIXME: Should not have to specify output of ""
}
