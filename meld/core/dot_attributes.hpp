#ifndef meld_core_dot_attributes_hpp
#define meld_core_dot_attributes_hpp

#include <string>

namespace meld::dot {
  struct attributes {
    std::string arrowtail{"none"};
    std::string label;
    std::string shape;
    std::string fontcolor{"black"};
  };

  template <typename Head, typename... Tail>
  std::string attributes_str(Head const& head, Tail const&... tail)
  {
    std::string result{" [" + head};
    (result.append(", " + tail), ...);
    result.append("]");
    return result;
  }

  inline std::string const default_fontsize{"12"};

#define DOT_ATTRIBUTE(name)                                                                        \
  inline std::string name(std::string const& str) { return #name "=" + str; }

  DOT_ATTRIBUTE(arrowtail)
  DOT_ATTRIBUTE(color)
  DOT_ATTRIBUTE(dir)
  DOT_ATTRIBUTE(fontsize)
  DOT_ATTRIBUTE(fontcolor)
  DOT_ATTRIBUTE(shape)
  DOT_ATTRIBUTE(style)

#undef DOT_ATTRIBUTE

  inline std::string label(std::string const& str) { return "label=\" " + str + '"'; }
}

#endif /* meld_core_dot_attributes_hpp */
