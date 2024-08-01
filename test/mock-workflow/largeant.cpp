#include "meld/module.hpp"
#include "meld/utilities/sized_tuple.hpp"
#include "test/mock-workflow/algorithm.hpp"
#include "test/mock-workflow/types.hpp"

#include <tuple>

using namespace meld::test;

DEFINE_MODULE(m, config)
{
  using assns = meld::association<simb::MCParticle, simb::MCTruth, sim::GeneratedParticleInfo>;
  using input = meld::sized_tuple<simb::MCTruths, 6>;
  using output = std::tuple<sim::ParticleAncestryMap,
                            assns,
                            sim::SimEnergyDeposits,
                            sim::AuxDetHits,
                            simb::MCParticles>;
  define_algorithm<input, output>(m, config);
}
