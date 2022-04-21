#ifndef meld_core_transition_hpp
#define meld_core_transition_hpp

#include <cstddef>
#include <utility>
#include <vector>

namespace meld {
  using id_t = std::vector<std::size_t>;
  enum class stage { setup, process };
  // std::string is the demangled name of the node type
  using transition_type = std::pair<std::string, stage>;
  using transition = std::pair<id_t, stage>;
  using transitions = std::vector<transition>;

  transitions transitions_between(id_t begin, id_t end);
  id_t operator"" _id(char const* str, std::size_t);

  std::string to_string(stage);
  std::ostream& operator<<(std::ostream& os, id_t const& id);
  std::ostream& operator<<(std::ostream& os, transition const& t);
}

#endif /* meld_core_transition_hpp */
