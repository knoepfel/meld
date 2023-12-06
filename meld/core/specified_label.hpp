#ifndef meld_core_specified_label_hpp
#define meld_core_specified_label_hpp

#include <iosfwd>
#include <memory>
#include <string>
#include <vector>

namespace meld {
  struct specified_label {
    std::string name;
    std::string domain;
    std::shared_ptr<specified_label> relation;
    specified_label operator()(std::string domain) &&;
    specified_label related_to(std::string relation) &&;
    specified_label related_to(specified_label relation) &&;
    std::string to_string() const;
  };

  inline auto& to_name(specified_label const& label) { return label.name; }
  inline auto& to_domain(specified_label& label) { return label.domain; }

  specified_label operator""_in(char const* str, std::size_t);
  bool operator==(specified_label const& a, specified_label const& b);
  bool operator!=(specified_label const& a, specified_label const& b);
  bool operator<(specified_label const& a, specified_label const& b);
  std::ostream& operator<<(std::ostream& os, specified_label const& label);

  template <typename T>
  concept label_compatible = requires(T t) {
                               {
                                 specified_label{t}
                               };
                             };
}

#endif /* meld_core_specified_label_hpp */
