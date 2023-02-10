#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

using namespace meld::concurrency;

namespace {
  void read_index(int) {}
}

DEFINE_MODULE(m, config)
{
  m.with(read_index).using_concurrency(unlimited).monitor(config.get<std::string>("product_name"));
}
