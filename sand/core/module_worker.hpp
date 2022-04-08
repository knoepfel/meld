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
    process(node& data)
    {
      do_process(data);
    }

  private:
    virtual void do_process(node&) = 0;
  };

  using module_creator_t = std::unique_ptr<module_worker>(boost::json::value const&);
}

#endif /* sand_core_module_worker_hpp */
