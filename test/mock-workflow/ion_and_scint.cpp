#include "meld/module.hpp"
#include "test/mock-workflow/algorithm.hpp"
#include "test/mock-workflow/types.hpp"

using namespace meld::test;

DEFINE_MODULE(m, config)
{
  define_algorithm<sim::SimEnergyDeposits, meld::sized_tuple<sim::SimEnergyDeposits, 2>>(m, config);
}
