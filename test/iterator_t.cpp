#include <cassert>
#include <chrono>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <numeric>
#include <vector>

namespace {

  template <typename T>
  class lazy_iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = T;
    using difference_type = std::ptrdiff_t;
    using pointer = T*;
    using reference = T const&;

    lazy_iterator() = default;
    explicit lazy_iterator(value_type const v) : value{v} {}

    reference operator*() const { return value; }
    pointer operator->() { return &operator*(); }
    lazy_iterator& operator++()
    {
      ++value;
      return *this;
    }
    lazy_iterator operator++(int)
    {
      auto old = *this;
      ++(*this);
      return old;
    }
    auto operator<=>(lazy_iterator const&) const = default;

  private:
    value_type value;
  };

  static_assert(std::forward_iterator<lazy_iterator<int>>);

  template <typename T>
  class container_with_lazy_iterators {
  public:
    container_with_lazy_iterators(T begin, T end) : begin_{begin}, end_{end} {}
    auto begin() const { return lazy_iterator{begin_}; }
    auto end() const { return lazy_iterator{end_}; }

  private:
    T begin_;
    T end_;
  };

  template <std::integral T>
  auto numbers_in_range(T begin, T end)
  {
    return container_with_lazy_iterators(begin, end);
  }

  class Record {
  public:
    explicit Record(std::size_t n) : apas(n) { std::iota(begin(apas), end(apas), 0); }
    Record(int b, int e) : begin_{b}, end_{e} {}
    template <typename T>
    auto& process_full() const
    {
      return apas;
    }
    template <typename T>
    auto process() const
    {
      return numbers_in_range(begin_, end_);
    }

  private:
    std::vector<int> apas;
    int begin_;
    int end_;
  };

}

int main()
{
  std::size_t sum [[maybe_unused]] = 0;

  bool const full = false;
  int n = 15'000;

  auto const begin_time = std::chrono::steady_clock::now();
  if (full) {
    // Eager evaluation
    Record trigger_record(n);
    for (int apa : trigger_record.process_full<int>()) {
      sum += apa;
    }
  }
  else {
    // Lazy evaluation
    Record trigger_record(0, n);
    for (int apa : trigger_record.process<int>()) {
      sum += apa;
    }
  }
  assert(sum == (n - 1) * n / 2);
  auto const end_time = std::chrono::steady_clock::now();
  std::cout << "Took " << (end_time - begin_time).count() << " ns\n";
}
