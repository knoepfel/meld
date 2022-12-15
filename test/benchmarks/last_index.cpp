#include "meld/graph/transition.hpp"
#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int last_index(level_id const& id) { return static_cast<int>(id.back()); }
}

DEFINE_MODULE(m, config)
{
  std::string product_name = "a";
  if (auto pname = config.if_contains("product_name")) {
    product_name = pname->as_string();
  }

  m.declare_transform("last_index", last_index)
    .concurrency(unlimited)
    .input("id")
    .output(product_name);
}
