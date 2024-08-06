#ifndef meld_core_specified_label_hpp
#define meld_core_specified_label_hpp

#include <algorithm>
#include <array>
#include <iosfwd>
#include <memory>
#include <span>
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

  using specified_labels = std::span<specified_label const, std::dynamic_extent>;
  using output_strings = std::span<std::string const, std::dynamic_extent>;

  inline auto& to_name(specified_label const& label) { return label.name; }
  inline auto& to_domain(specified_label& label) { return label.domain; }

  specified_label operator""_in(char const* str, std::size_t);
  bool operator==(specified_label const& a, specified_label const& b);
  bool operator!=(specified_label const& a, specified_label const& b);
  bool operator<(specified_label const& a, specified_label const& b);
  std::ostream& operator<<(std::ostream& os, specified_label const& label);

  template <typename T>
  concept label_compatible = requires(T t) {
    { specified_label{t} };
  };

  template <label_compatible T, std::size_t N>
  auto to_labels(std::array<T, N> const& like_labels)
  {
    std::array<specified_label, N> labels;
    std::ranges::transform(
      like_labels, labels.begin(), [](T const& t) { return specified_label{t}; });
    return labels;
  }

}

#endif /* meld_core_specified_label_hpp */
