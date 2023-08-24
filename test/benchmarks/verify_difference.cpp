#include "meld/module.hpp"

#include <cassert>

using namespace meld;
using namespace meld::concurrency;

DEFINE_MODULE(m, config)
{
  m.with("verify_difference",
         [expected = config.get<int>("expected", 100)](int i, int j) { assert(j - i == expected); })
    .monitor(config.get<std::string>("i", "b"), config.get<std::string>("j", "c"))
    .using_concurrency(unlimited);
}
