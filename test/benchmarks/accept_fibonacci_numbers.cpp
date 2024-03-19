#include "meld/module.hpp"
#include "test/benchmarks/fibonacci_numbers.hpp"

#include <string>

DEFINE_MODULE(m, config)
{
  m.make<test::fibonacci_numbers>(config.get<int>("max_number"))
    .with(&test::fibonacci_numbers::accept, meld::concurrency::unlimited)
    .evaluate(config.get<std::string>("consumes"));
}
