#include "meld/module.hpp"

#include <string>

using namespace meld::concurrency;

namespace {
  bool accept_even_numbers(int const i) { return i % 2 == 0; }
}

DEFINE_MODULE(m, config)
{
  m.declare_filter(accept_even_numbers)
    .concurrency(unlimited)
    .react_to(config.get<std::string>("product_name"));
}
