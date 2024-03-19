#ifndef meld_core_consumer_hpp
#define meld_core_consumer_hpp

#include <string>
#include <vector>

namespace meld {
  class consumer {
  public:
    consumer(std::string name, std::vector<std::string> predicates);

    std::string const& name() const noexcept;
    std::vector<std::string> const& when() const noexcept;

  private:
    std::string name_;
    std::vector<std::string> predicates_;
  };
}

#endif /* meld_core_consumer_hpp */
