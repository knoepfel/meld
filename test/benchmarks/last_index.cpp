#include "meld/model/transition.hpp"
#include "meld/module.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  int last_index(level_id const& id) { return static_cast<int>(id.back()); }
}

DEFINE_MODULE(m, config)
{
  m.declare_transform(last_index)
    .concurrency(unlimited)
    .react_to("id")
    .output(config.get<std::string>("product_name", "a"));
}
