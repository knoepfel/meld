#include "meld/graph/transition.hpp"
#include "meld/module.hpp"

using namespace meld::concurrency;

namespace {
  void read_index(int) {}
}

DEFINE_MODULE(m, config)
{
  m.declare_monitor("read_index", read_index)
    .concurrency(unlimited)
    .input(value_to<std::string>(config.at("product_name")));
}
