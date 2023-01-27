#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

using namespace meld::concurrency;

namespace {
  void read_id(meld::level_id const&) {}
}

DEFINE_MODULE(m) { m.declare_monitor(read_id).concurrency(unlimited).react_to("id"); }
