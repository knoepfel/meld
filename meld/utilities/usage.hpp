#ifndef meld_utilities_usage_hpp
#define meld_utilities_usage_hpp

// =======================================================================================
// The usage class tracks the CPU time and real time during the lifetime of a usage
// object.  The destructor will also report the maximum RSS of the process.
// =======================================================================================

#include <chrono>

namespace meld {
  class usage {
  public:
    usage() noexcept;
    ~usage();

  private:
    std::chrono::time_point<std::chrono::steady_clock> begin_wall_;
    double begin_cpu_;
  };
}

#endif /* meld_utilities_usage_hpp */
