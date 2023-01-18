#include "meld/module.hpp"
#include "test/products_for_output.hpp"

using namespace meld::concurrency;
using namespace meld::test;

DEFINE_MODULE(m)
{
  m.make<products_for_output>().declare_output(&products_for_output::save).concurrency(unlimited);
}
