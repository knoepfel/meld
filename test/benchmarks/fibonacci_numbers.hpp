#ifndef test_benchmarks_fibonacci_numbers_hpp
#define test_benchmarks_fibonacci_numbers_hpp

#include <vector>

namespace test {
  class fibonacci_numbers {
  public:
    explicit fibonacci_numbers(int n);
    bool accept(int i) const;

  private:
    std::vector<int> numbers_;
  };
}

#endif /* test_benchmarks_fibonacci_numbers_hpp */
