#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int plus_101(int i) noexcept { return i + 101; }
}

DEFINE_MODULE(m) { m.with(plus_101).transform("a").to("c").using_concurrency(unlimited); }
