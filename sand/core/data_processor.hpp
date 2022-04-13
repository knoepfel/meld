#ifndef sand_core_data_processor_hpp
#define sand_core_data_processor_hpp

#include "sand/core/module_owner.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/source_owner.hpp"
#include "sand/core/source_worker.hpp"
#include "sand/core/task_scheduler.hpp"

#include <vector>

namespace sand {

  class data_processor {
  public:
    explicit data_processor(std::unique_ptr<source_worker> sworker,
                            std::vector<module_worker_ptr> mworkers) :
      source_{move(sworker)}, workers_{move(mworkers)}
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
      for (auto& w : workers_) {
        w->process(data);
      }
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
    std::vector<module_worker_ptr> workers_;
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

#endif /* sand_core_data_processor_hpp */
