#ifndef meld_core_module_worker_hpp
#define meld_core_module_worker_hpp

#include "meld/core/node.hpp"

#include "boost/json.hpp"

#include <memory>

namespace meld {
  class module_worker {
  public:
    virtual ~module_worker();

    std::vector<transition_type> supported_transitions() const;
    void process(stage const s, node& data);
    std::vector<std::string> dependencies() const;

  private:
    virtual void do_process(stage, node&) = 0;
    virtual std::vector<std::string> required_dependencies() const = 0;
    virtual std::vector<transition_type> supported_setup_transitions() const = 0;
    virtual std::vector<transition_type> supported_process_transitions() const = 0;
  };

  using module_worker_ptr = std::unique_ptr<module_worker>;
  using module_creator_t = module_worker_ptr(boost::json::value const&);
}

#endif /* meld_core_module_worker_hpp */