#ifndef meld_utilities_sleep_for_hpp
#define meld_utilities_sleep_for_hpp

#include <chrono>
#include <thread>

namespace meld {
  template <typename T>
  void
  sleep_for(T duration)
  {
    std::this_thread::sleep_for(duration);
  }

  // Risky...
  using namespace std::chrono_literals;
}

#endif /* meld_utilities_sleep_for_hpp */
