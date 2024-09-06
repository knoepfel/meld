#ifndef test_mock_workflow_timed_busy_hpp
#define test_mock_workflow_timed_busy_hpp

#include <chrono>

namespace meld::test {
  void timed_busy(std::chrono::microseconds const& duration);
}

#endif // test_mock_workflow_timed_busy_hpp
