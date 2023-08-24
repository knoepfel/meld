#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int plus_one(int i) noexcept { return i + 1; }
}

DEFINE_MODULE(m) { m.with(plus_one).transform("a").to("b").using_concurrency(unlimited); }
