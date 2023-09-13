#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

namespace {
  void read_index(int) {}
}

DEFINE_MODULE(m, config)
{
  m.with(read_index, meld::concurrency::unlimited).monitor(config.get<std::string>("consumes"));
}
