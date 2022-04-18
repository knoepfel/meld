#ifndef sand_core_task_scheduler_hpp
#define sand_core_task_scheduler_hpp

#include "oneapi/tbb/task_group.h"

#include <functional>
#include <list>

namespace sand {
  class task_scheduler {
  public:
    void
    append_task(std::function<void()> task)
    {
      tasks_.push_back(move(task));
    }

    void
    run()
    {
      run_();
      group_.wait();
    }

  private:
    void
    run_()
    {
      if (empty(tasks_))
        return;

      auto task = tasks_.front();
      tasks_.pop_front();
      task();
      group_.run([this] { run_(); });
    }

    std::list<std::function<void()>> tasks_;
    tbb::task_group group_;
  };
}

#endif /* sand_core_task_scheduler_hpp */
