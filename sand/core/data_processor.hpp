#ifndef sand_core_data_processor_h
#define sand_core_data_processor_h

#include "sand/core/load_module.hpp"
#include "sand/core/module_owner.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/source_owner.hpp"
#include "sand/core/source_worker.hpp"
#include "sand/core/task_scheduler.hpp"

namespace sand {

  class data_processor {
  public:
    explicit data_processor(std::size_t const n,
                            std::unique_ptr<source_worker> sworker = nullptr,
                            std::unique_ptr<module_worker> mworker = nullptr) :
      source_{sworker ? std::move(sworker) : load_source("test_source", n)},
      worker_{mworker ? std::move(mworker) : load_module("test_module")}
    {
    }

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
      if (auto data = source_->next()) {
        scheduler_.append_task([d = data, this] { process(*d); });
        scheduler_.append_task([this] { next(); });
      }
    }

    std::unique_ptr<source_worker> source_;
    std::unique_ptr<module_worker> worker_;
    task_scheduler scheduler_{};
  };

  // Intended to help with testing
  template <typename Source, typename Module>
  class data_processor_for : public data_processor {
  public:
    explicit data_processor_for(std::size_t const n) :
      data_processor{std::make_unique<source_owner<Module>>(n),
                     std::make_unique<module_owner<Module>>()}
    {
    }
  };
}

#endif
