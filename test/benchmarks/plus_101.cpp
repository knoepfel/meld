#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int plus_101(int i) noexcept { return i + 101; }
}

DEFINE_MODULE(m) { m.declare_transform(plus_101).concurrency(unlimited).consumes("a").output("c"); }
