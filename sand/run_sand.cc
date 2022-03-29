#include "sand/run_sand.hh"
#include "sand/core/source.hh"
#include "sand/core/task_scheduler.hh"

#include <functional>
#include <iostream>

namespace sand {
  void
  process(node& data)
  {
    std::cout << "Processing data "
              << "(" << data.id() << ")\n";
  }

  struct DataProcessor {
    explicit DataProcessor(std::size_t const n, task_scheduler& scheduler) :
      source_{n}, scheduler_{scheduler}
    {
    }

    void
    execute()
    {
      if (auto data = source_.next()) {
        scheduler_.append_task([d = data] { process(*d); });
        scheduler_.append_task([this] { execute(); });
      }
    }

    source source_;
    task_scheduler& scheduler_;
  };

  void
  run(std::size_t const n)
  {
    std::cout << "Running sand\n";
    task_scheduler scheduler{};
    DataProcessor processor{n, scheduler};
    scheduler.append_task([&processor] { processor.execute(); });
    scheduler.run();
  }
}
