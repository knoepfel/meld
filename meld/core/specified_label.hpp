#ifndef meld_core_specified_label_hpp
#define meld_core_specified_label_hpp

#include <string>
#include <vector>

namespace meld {
  struct specified_label {
    std::string name;
    std::string domain;
    specified_label operator()(std::string domain) &&;
  };

  specified_label operator""_in(char const* str, std::size_t);
  bool operator==(specified_label const& a, specified_label const& b);
  bool operator!=(specified_label const& a, specified_label const& b);
  bool operator<(specified_label const& a, specified_label const& b);

  template <typename T>
  concept label_compatible = requires(T t) {
    {
      specified_label{t}
    };
  };
}

#endif /* meld_core_specified_label_hpp */
