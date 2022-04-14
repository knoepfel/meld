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
    process(node& data)
    {
      for (auto& pr : modules_->modules()) {
        pr.second->process(data);
      }
    }

    void
    next()
    {
      if (auto data = modules_->source().next()) {
        scheduler_.append_task([d = data, this] { process(*d); });
        scheduler_.append_task([this] { next(); });
      }
    }

    module_manager* modules_;
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
