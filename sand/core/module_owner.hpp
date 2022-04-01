#ifndef sand_core_module_owner_hpp
#define sand_core_module_owner_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"

#include <cstring>
#include <memory>
#include <typeinfo>

namespace sand {
  template <typename T, typename... Ds>
  class module_owner : public module_worker {
  public:
    static std::unique_ptr<module_worker>
    create()
    {
      return std::make_unique<module_owner<T, Ds...>>();
    }

  private:
    template <typename D>
    void
    process_(node& data)
    {
      // FIXME: This is bad, really bad.  I should only need to
      //        compare typeids.  However, I'm having trouble getting
      //        the typeids to compare equivalent when creating an
      //        object of one type in a one plugin (the source) and
      //        reading that same object in a different plugin (a
      //        module).  This may something to do with the linker I'm
      //        using on my macOS (ld).
      if (strcmp(typeid(data).name(), typeid(D).name()) != 0) {
        return;
      }
      user_module.process(static_cast<D&>(data));
    }

    void
    do_process(node& data) final
    {
      (process_<Ds>(data), ...);
    }

    T user_module;
  };
}

#endif
