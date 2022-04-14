#ifndef sand_core_source_owner_hpp
#define sand_core_source_owner_hpp

#include "sand/core/node.hpp"
#include "sand/core/source_worker.hpp"
#include "sand/core/uses_config.hpp"

#include <memory>

namespace sand {
  template <typename T>
  class source_owner : public source_worker {
  public:
    template <with_config U = T>
    explicit source_owner(boost::json::object const& config) : user_source{config}
    {
    }

    template <without_config U = T>
    explicit source_owner(boost::json::object const&) : user_source{}
    {
    }

    static std::unique_ptr<source_worker>
    create(boost::json::object const& config)
    {

      return std::make_unique<source_owner<T>>(config);
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
