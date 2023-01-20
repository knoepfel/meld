#ifndef meld_core_consumer_hpp
#define meld_core_consumer_hpp

#include <string>
#include <vector>

namespace meld {
  class consumer {
  public:
    consumer(std::string name,
             std::vector<std::string> preceding_filters,
             std::vector<std::string> receive_stores);

    std::string const& name() const noexcept;
    std::vector<std::string> const& filtered_by() const noexcept;
    std::vector<std::string> const& receive_stores() const noexcept;

  private:
    std::string name_;
    std::vector<std::string> preceding_filters_;
    std::vector<std::string> receive_stores_;
  };
}

#endif /* meld_core_consumer_hpp */
