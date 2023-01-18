#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  void verify_difference(int i, int j, int expected) noexcept { assert(j - i == expected); }
}

DEFINE_MODULE(m, config)
{
  int expected{100};
  std::string i{"b"};
  std::string j{"c"};
  if (auto exp = config.if_contains("expected")) {
    expected = exp->as_int64();
  }
  if (auto i_config = config.if_contains("i")) {
    i = value_to<std::string>(*i_config);
  }
  if (auto j_config = config.if_contains("j")) {
    j = value_to<std::string>(*j_config);
  }

  m.declare_monitor(verify_difference).concurrency(unlimited).input(i, j, use(expected));
}
