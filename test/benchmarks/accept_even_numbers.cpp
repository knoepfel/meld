#include "meld/module.hpp"

#include <string>

using namespace meld::concurrency;

DEFINE_MODULE(m, config)
{
  m.with("accept_even_numbers", [](int i) { return i % 2 == 0; })
    .filter(config.get<std::string>("product_name"))
    .using_concurrency(unlimited);
}
