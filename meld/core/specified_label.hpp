#ifndef meld_core_specified_label_hpp
#define meld_core_specified_label_hpp

#include <iosfwd>
#include <string>
#include <vector>

namespace meld {
  struct specified_label {
    std::string name;
    std::string domain;
    specified_label operator()(std::string domain) &&;
    std::string to_string() const;
  };

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
