#include "test/mock-workflow/timed_busy.hpp"

void meld::test::timed_busy(std::chrono::microseconds const& duration)
{
  using namespace std::chrono;
  auto const stop = steady_clock::now() + duration;
  while (steady_clock::now() < stop) {
    // Do nothing
  }
}
