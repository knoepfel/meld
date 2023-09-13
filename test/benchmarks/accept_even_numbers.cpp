#include "meld/module.hpp"

#include <string>

DEFINE_MODULE(m, config)
{
  m.with(
     "accept_even_numbers", [](int i) { return i % 2 == 0; }, meld::concurrency::unlimited)
    .filter(config.get<std::string>("consumes"));
}
