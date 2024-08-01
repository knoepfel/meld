#include "meld/module.hpp"
#include "test/mock-workflow/algorithm.hpp"
#include "test/mock-workflow/types.hpp"

#include <tuple>

using namespace meld::test;

DEFINE_MODULE(m, config)
{
  using inputs = meld::level_id;
  using outputs = std::tuple<simb::MCTruths, beam::ProtoDUNEBeamEvents, sim::ProtoDUNEbeamsims>;
  define_algorithm<inputs, outputs>(m, config);
}
