#ifndef sand_core_module_worker_hpp
#define sand_core_module_worker_hpp

#include "sand/core/node.hpp"

#include "boost/json.hpp"

#include <memory>

namespace sand {
  class module_worker {
  public:
    virtual ~module_worker() = default;
    void
    process(stage const s, node& data)
    {
      do_process(s, data);
    }

  private:
    virtual void do_process(stage, node&) = 0;
  };

  using module_worker_ptr = std::unique_ptr<module_worker>;
  using module_creator_t = module_worker_ptr(boost::json::value const&);
}

#endif /* sand_core_module_worker_hpp */
