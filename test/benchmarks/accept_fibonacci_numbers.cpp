#include "meld/module.hpp"

#include <algorithm>
#include <string>

namespace test {
  auto fibs_less_than(int const n)
  {
    std::vector<int> result;
    int i = 0;
    int j = 1;
    int sum = 0;
    while (sum < n) {
      result.push_back(sum);
      sum = i + j;
      i = j;
      j = sum;
    }
    return result;
  }

  class fibonacci_numbers {
  public:
    explicit fibonacci_numbers(int const n) : numbers_{fibs_less_than(n + 1)} {}
    bool accept(int i) const { return std::binary_search(begin(numbers_), end(numbers_), i); }

  private:
    std::vector<int> numbers_;
  };
}

DEFINE_MODULE(m, config)
{
  m.make<test::fibonacci_numbers>(config.get<int>("max_number"))
    .with(&test::fibonacci_numbers::accept, meld::concurrency::unlimited)
    .filter(config.get<std::string>("consumes"));
}
