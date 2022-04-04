#ifndef sand_core_source_owner_hpp
#define sand_core_source_owner_hpp

#include "sand/core/node.hpp"
#include "sand/core/source_worker.hpp"

#include <memory>

namespace sand {
  template <typename T>
  class source_owner : public source_worker {
  public:
    explicit source_owner(std::size_t const n) : user_source{n} {}

    static std::unique_ptr<source_worker>
    create(std::size_t const n)
    {
      return std::make_unique<source_owner<T>>(n);
    }

  private:
    std::shared_ptr<node>
    data() final
    {
      return user_source.data();
    }

    T user_source;
  };
}

#endif /* sand_core_source_owner_hpp */
