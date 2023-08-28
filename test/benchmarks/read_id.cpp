#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

namespace {
  void read_id(meld::level_id const&) {}
}

DEFINE_MODULE(m) { m.with(read_id, meld::concurrency::unlimited).monitor("id"); }
