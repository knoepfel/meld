#ifndef sand_core_source_hh
#define sand_core_source_hh

#include "sand/core/node.hh"

namespace sand {
  class source {
  public:
    virtual ~source() = default;
    std::unique_ptr<node> next();
  };
}

#define
