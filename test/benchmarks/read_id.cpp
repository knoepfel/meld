#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

using namespace meld::concurrency;

namespace {
  void read_id(meld::level_id const&) {}
}

DEFINE_MODULE(m) { m.with(read_id).monitor("id").using_concurrency(unlimited); }
