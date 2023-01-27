#ifndef meld_model_fwd_hpp
#define meld_model_fwd_hpp

#include <utility>
#include <vector>

namespace meld {
  class level_counter;
  class level_hierarchy;
  class level_id;
  class level_order;
  class product_store;
  class product_store_factory;

  enum class stage { process, flush };

  using transition = std::pair<level_id, stage>;
  using transitions = std::vector<transition>;
}

#endif /* meld_model_fwd_hpp */
