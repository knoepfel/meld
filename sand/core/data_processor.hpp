#ifndef sand_core_data_processor_hpp
#define sand_core_data_processor_hpp

#include "sand/core/module_manager.hpp"
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
    explicit data_processor(module_manager* modules) : modules_{modules} {}

    void
    run_to_completion()
    {
      scheduler_.append_task([this] { next(); });
      scheduler_.run();
    }

  private:
    void
    process(transition_packages& packages)
    {
      for (auto& [stage, node] : packages) {
        for (auto& pr : modules_->modules()) {
          pr.second->process(stage, *node);
        }
      }
    }

    void
    next()
    {
      auto data = modules_->source().next();
      if (empty(data)) {
        return;
      }

      scheduler_.append_task([d = std::move(data), this]() mutable { process(d); });
      scheduler_.append_task([this] { next(); });
    }

    module_manager* modules_;
    task_scheduler scheduler_{};
  };
}

#endif /* sand_core_data_processor_hpp */
