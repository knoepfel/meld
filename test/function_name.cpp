#include "meld/metaprogramming/function_name.hpp"

#include "catch2/catch_all.hpp"

#include <utility>

using namespace std::string_literals;
using namespace meld;

namespace {
  void a(int) {}
}

static std::pair<int, double> b() { return {}; }

namespace c::d::e {
  struct f {
    int g(double, char) const { return 0; }
    static void h() {}
  };
}

namespace {
  struct i {};
  double j(i) { return 0; }
}

namespace k {
  namespace {
    struct l {};
    void m(l) {}
  }
}

TEST_CASE("Test function names", "[metaprogramming]")
{
  CHECK(function_name(a) == "a");
  CHECK(function_name(::b) == "b");
  CHECK(function_name(&c::d::e::f::g) == "g");
  CHECK(function_name(&c::d::e::f::h) == "h");
  CHECK(function_name(j) == "j");
  CHECK(function_name(k::m) == "m");
}
