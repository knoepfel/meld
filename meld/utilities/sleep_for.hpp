#ifndef meld_utilities_sleep_for_hpp
#define meld_utilities_sleep_for_hpp

#include <chrono>
#include <thread>

namespace meld {
  template <typename T>
  void sleep_for(T duration)
  {
    std::this_thread::sleep_for(duration);
  }

  template <typename T>
  void spin_for(T duration)
  {
    using namespace std::chrono;
    auto start = steady_clock::now();
    while (duration_cast<T>(steady_clock::now() - start) < duration) {}
  }

  // Risky...
  using namespace std::chrono_literals;
}

#endif // meld_utilities_sleep_for_hpp
