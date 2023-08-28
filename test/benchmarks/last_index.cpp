#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

using namespace meld;

namespace {
  int last_index(level_id const& id) { return static_cast<int>(id.number()); }
}

DEFINE_MODULE(m, config)
{
  m.with(last_index, concurrency::unlimited)
    .transform("id")
    .to(config.get<std::string>("product_name", "a"));
}
