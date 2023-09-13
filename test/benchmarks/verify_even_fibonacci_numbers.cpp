#include "meld/module.hpp"
#include "test/benchmarks/fibonacci_numbers.hpp"

#include <cassert>

namespace test {
  class even_fibonacci_numbers {
  public:
    explicit even_fibonacci_numbers(int const n) : numbers_(n) {}
    void only_even(int const n) const { assert(n % 2 == 0 and numbers_.accept(n)); }

  private:
    fibonacci_numbers numbers_;
  };
}

DEFINE_MODULE(m, config)
{
  using namespace test;
  m.make<even_fibonacci_numbers>(config.get<int>("max_number"))
    .with(&even_fibonacci_numbers::only_even, meld::concurrency::unlimited)
    .monitor(config.get<std::string>("consumes"));
}
