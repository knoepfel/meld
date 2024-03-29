#include "meld/model/level_id.hpp"
#include "meld/module.hpp"

#include <string>

DEFINE_MODULE(m, config)
{
  m.with(
     "accept_even_ids",
     [](meld::level_id const& id) { return id.number() % 2 == 0; },
     meld::concurrency::unlimited)
    .evaluate(config.get<std::string>("product_name"));
}
