#include "meld/module.hpp"
#include "test/mock-workflow/algorithm.hpp"
#include "test/mock-workflow/types.hpp"

using namespace meld::test;

DEFINE_MODULE(m, config) { define_algorithm<meld::level_id, simb::MCTruths>(m, config); }
