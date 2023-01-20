#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  void verify_difference(int i, int j, int expected) noexcept { assert(j - i == expected); }
}

DEFINE_MODULE(m, config)
{
  m.declare_monitor(verify_difference)
    .concurrency(unlimited)
    .input(consumes(config.get<std::string>("i", "b")),
           consumes(config.get<std::string>("j", "c")),
           use(config.get<int>("expected", 100)));
}
