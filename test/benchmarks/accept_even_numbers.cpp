#include "meld/module.hpp"

#include <string>

using namespace meld::concurrency;

namespace {
  bool accept_even_numbers(int const i) { return i % 2 == 0; }
}

DEFINE_MODULE(m, config)
{
  m.declare_filter("accept_even_numbers", accept_even_numbers)
    .concurrency(unlimited)
    .input(value_to<std::string>(config.at("product_name")));
}
