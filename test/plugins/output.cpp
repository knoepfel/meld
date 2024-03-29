#include "meld/module.hpp"
#include "test/products_for_output.hpp"

using namespace meld::test;

DEFINE_MODULE(m)
{
  m.make<products_for_output>().output_with(&products_for_output::save,
                                            meld::concurrency::unlimited);
}
