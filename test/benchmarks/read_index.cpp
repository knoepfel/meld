#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

using namespace meld::concurrency;

namespace {
  void read_index(int) {}
}

DEFINE_MODULE(m, config)
{
  m.declare_monitor(read_index)
    .concurrency(unlimited)
    .react_to(config.get<std::string>("product_name"));
}
