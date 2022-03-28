#ifndef sand_core_module_hh
#define sand_core_module_hh

#include "sand/core/node.hh"

namespace sand {
  template <typename T>
  class module {
  public:
    void
    process(node& data)
    {
      user_module.process(static_cast<T&>(data));
    }

  private:
    T user_module;
  };
}

#define
