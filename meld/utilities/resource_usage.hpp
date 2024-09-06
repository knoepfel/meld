#ifndef meld_utilities_resource_usage_hpp
#define meld_utilities_resource_usage_hpp

// =======================================================================================
// The resource_usage class tracks the CPU time and real time during the lifetime of a
// resource_usage object.  The destructor will also report the maximum RSS of the process.
// =======================================================================================

#include <chrono>

namespace meld {
  class resource_usage {
  public:
    resource_usage() noexcept;
    ~resource_usage();

  private:
    std::chrono::time_point<std::chrono::steady_clock> begin_wall_;
    double begin_cpu_;
  };
}

#endif // meld_utilities_resource_usage_hpp
