#ifndef meld_core_node_catalog_hpp
#define meld_core_node_catalog_hpp

#include "meld/core/declared_monitor.hpp"
#include "meld/core/declared_output.hpp"
#include "meld/core/declared_predicate.hpp"
#include "meld/core/declared_reduction.hpp"
#include "meld/core/declared_splitter.hpp"
#include "meld/core/declared_transform.hpp"
#include "meld/core/registrar.hpp"

namespace meld {
  struct node_catalog {
    auto register_predicate(std::vector<std::string>& errors)
    {
      return registrar{predicates_, errors};
    }
    auto register_monitor(std::vector<std::string>& errors) { return registrar{monitors_, errors}; }
    auto register_output(std::vector<std::string>& errors) { return registrar{outputs_, errors}; }
    auto register_reduction(std::vector<std::string>& errors)
    {
      return registrar{reductions_, errors};
    }
    auto register_splitter(std::vector<std::string>& errors)
    {
      return registrar{splitters_, errors};
    }
    auto register_transform(std::vector<std::string>& errors)
    {
      return registrar{transforms_, errors};
    }

    declared_predicates predicates_{};
    declared_monitors monitors_{};
    declared_outputs outputs_{};
    declared_reductions reductions_{};
    declared_splitters splitters_{};
    declared_transforms transforms_{};
  };
}

#endif /* meld_core_node_catalog_hpp */
