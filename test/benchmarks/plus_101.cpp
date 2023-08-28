#include "meld/module.hpp"

using namespace meld;

namespace {
  int plus_101(int i) noexcept { return i + 101; }
}

DEFINE_MODULE(m) { m.with(plus_101, concurrency::unlimited).transform("a").to("c"); }
