#include "meld/module.hpp"
#include "test/mock-workflow/algorithm.hpp"
#include "test/mock-workflow/types.hpp"

using namespace meld::test;

DEFINE_MODULE(m, config)
{
  define_algorithm<sim::SimEnergyDeposits,
                   std::tuple<sim::SimPhotonLites, sim::OpDetBacktrackerRecords>>(m, config);
}
