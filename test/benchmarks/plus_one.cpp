#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int plus_one(int i) noexcept { return i + 1; }
}

DEFINE_MODULE(m) { m.declare_transform(plus_one).concurrency(unlimited).input("a").output("b"); }
