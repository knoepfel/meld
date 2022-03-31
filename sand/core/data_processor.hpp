#ifndef sand_core_data_processor_h
#define sand_core_data_processor_h

#include "sand/core/load_module.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/task_scheduler.hpp"

namespace sand {

  template <typename Source>
  class data_processor {
  public:
    explicit data_processor(std::size_t const n) : source_{n}, worker_{load_module()} {}

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
      worker_->process(data);
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
    std::unique_ptr<module_worker> worker_;
    task_scheduler scheduler_{};
  };

}

#endif
