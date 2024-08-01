#ifndef test_mock_workflow_algorithm_hpp
#define test_mock_workflow_algorithm_hpp

#include "meld/concurrency.hpp"
#include "meld/configuration.hpp"
#include "test/mock-workflow/timed_busy.hpp"

#include <array>
#include <chrono>
#include <concepts>
#include <string>
#include <tuple>

namespace meld::test {
  template <typename T>
  constexpr std::size_t output_size = 1ull;

  template <typename... Ts>
  constexpr std::size_t output_size<std::tuple<Ts...>> = sizeof...(Ts);

  template <typename Outputs>
  inline Outputs after_busy(std::chrono::microseconds const& duration)
  {
    timed_busy(duration);
    return Outputs{};
  }

  template <typename Input, std::default_initializable Outputs>
  class algorithm {
  public:
    explicit algorithm(unsigned const duration) : duration_{duration} {}
    Outputs execute(Input const&) const { return after_busy<Outputs>(duration_); }

    using inputs = std::array<std::string, 1ull>;
    using outputs = std::array<std::string, output_size<Outputs>>;

  private:
    std::chrono::microseconds duration_;
  };

  template <typename... Inputs, std::default_initializable Outputs>
  class algorithm<std::tuple<Inputs...>, Outputs> {
  public:
    explicit algorithm(unsigned const duration) : duration_{duration} {}
    Outputs execute(Inputs const&...) const { return after_busy<Outputs>(duration_); }

    using inputs = std::array<std::string, sizeof...(Inputs)>;
    using outputs = std::array<std::string, output_size<Outputs>>;

  private:
    std::chrono::microseconds duration_;
  };

  template <typename Inputs, typename Outputs, typename M>
  void define_algorithm(M& m, configuration const& c)
  {
    using algorithm_t = algorithm<Inputs, Outputs>;
    concurrency const j{c.get<unsigned>("concurrency", concurrency::unlimited.value)};
    m.template make<algorithm_t>(c.get<unsigned>("duration_usec"))
      .with(&algorithm_t::execute, j)
      .transform(c.get<typename algorithm_t::inputs>("inputs"))
      .to(c.get<typename algorithm_t::outputs>("outputs"));
  }
}

#endif /* test_mock_workflow_algorithm_hpp */
