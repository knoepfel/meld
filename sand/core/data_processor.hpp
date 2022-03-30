#ifndef sand_core_data_processor_h
#define sand_core_data_processor_h

#include "sand/core/node.hpp"
#include "sand/core/task_scheduler.hpp"

#include <iostream>

namespace sand {
  template <typename Source, typename Module>
  class data_processor {
  public:
    explicit data_processor(std::size_t const n) : source_{n} {}

    void
    run_to_completion()
    {
      scheduler_.append_task([this] { next(); });
      scheduler_.run();
    }

  private:
    void
    process(node& data)
    {
      std::cout << "Processing data "
                << "(" << data.id() << ")\n";
      module_.process(data);
    }

    void
    next()
    {
      if (auto data = source_.next()) {
        scheduler_.append_task([d = data, this] { process(*d); });
        scheduler_.append_task([this] { next(); });
      }
    }

    Source source_;
    Module module_;
    task_scheduler scheduler_{};
  };

}

#endif
