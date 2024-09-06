#ifndef meld_core_consumer_hpp
#define meld_core_consumer_hpp

#include "meld/model/qualified_name.hpp"

#include <string>
#include <vector>

namespace meld {
  class consumer {
  public:
    consumer(qualified_name name, std::vector<std::string> predicates);

    std::string full_name() const;
    std::string const& module() const noexcept;
    std::string const& name() const noexcept;
    std::vector<std::string> const& when() const noexcept;

  private:
    qualified_name name_;
    std::vector<std::string> predicates_;
  };
}

#endif // meld_core_consumer_hpp
