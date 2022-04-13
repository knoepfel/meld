#ifndef sand_core_data_levels_hpp
#define sand_core_data_levels_hpp

// Everything inside of here is considered "experiment-defined".

#include "sand/core/node.hpp"

namespace sand {
  class run : public node {
  public:
    using node::node;
    ~run() final;
  };

  class subrun : public node {
  public:
    using node::node;
    ~subrun() final;
  };
}

#endif /* sand_core_data_levels_hpp */
