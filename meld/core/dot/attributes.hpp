#ifndef meld_core_dot_attributes_hpp
#define meld_core_dot_attributes_hpp

#include <string>

namespace meld::dot {
  struct attributes {
    std::string color;
    std::string fontcolor;
    std::string fontsize;
    std::string label;
    std::string shape;
    std::string style;
  };

  inline std::string const default_fontsize{"12"};

  std::string to_string(attributes const& attrs);
  std::string parenthesized(std::string const& n);
}

#endif // meld_core_dot_attributes_hpp
