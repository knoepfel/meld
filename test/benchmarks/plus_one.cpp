#include "meld/module.hpp"

using namespace meld;

namespace {
  int plus_one(int i) noexcept { return i + 1; }
}

DEFINE_MODULE(m) { m.with(plus_one, concurrency::unlimited).transform("a").to("b"); }
