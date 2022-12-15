#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int plus_101(int i) noexcept { return i + 101; }
}

DEFINE_MODULE(m)
{
  m.declare_transform("c_creator", plus_101).concurrency(unlimited).input("a").output("c");
}
