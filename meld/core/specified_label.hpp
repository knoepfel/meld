#ifndef meld_core_specified_label_hpp
#define meld_core_specified_label_hpp

#include <string>
#include <vector>

namespace meld {
  struct specified_label {
    std::string name;
    std::vector<std::string> allowed_domains;

    specified_label operator()(std::string domain) &&;
  };

  specified_label operator""_in_each(char const* str, std::size_t);
  bool operator==(specified_label const& a, specified_label const& b);
  bool operator!=(specified_label const& a, specified_label const& b);
}

#endif /* meld_core_specified_label_hpp */
