#ifndef sand_core_module_owner_hpp
#define sand_core_module_owner_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/node.hpp"
#include "sand/core/uses_config.hpp"

#include <memory>
#include <type_traits>

namespace sand {
  template <typename D, typename... Ds, typename Parent = typename D::parent_type>
  consteval bool
  supports_parent()
  {
    if (std::is_same<Parent, null_node_t>()) {
      return false;
    }
    return (std::is_same<Parent, Ds>() || ...);
  }

  template <typename T, typename D>
  concept has_process = requires(T t, D const& d)
  {
    // clang-format off
    {t.process(d)} -> std::same_as<void>;
    // clang-format on
  };

  template <typename T, typename D>
  concept has_setup = requires(T t, D const& d)
  {
    // clang-format off
    {t.setup(d)} -> std::same_as<void>;
    // clang-format on
  };

  template <typename T, typename... Ds>
  class module_owner : public module_worker {
  public:
    template <with_config U = T>
    explicit module_owner(boost::json::object const& config) : user_module{config}
    {
    }

    template <without_config U = T>
    explicit module_owner(boost::json::object const&) : user_module{}
    {
    }

    static std::unique_ptr<module_worker>
    create(boost::json::object const& config)
    {
      return std::make_unique<module_owner<T, Ds...>>(config);
    }

    // For testing
    T const&
    module() const noexcept
    {
      return user_module;
    }

  private:
    // template <typename D>
    // bool
    // must_process(D*) const
    // {
    //   return false;
    // }

    template <typename D>
    bool
    setup(node& data)
    {
      if constexpr (!has_setup<T, D>) {
        return true;
      }
      else {
        if (auto d = dynamic_cast<D*>(&data)) {
          user_module.setup(*d);
          return true;
        }
        return false;
      }
    }

    template <typename D>
    bool
    process(node& data)
    {
      if constexpr (!has_process<T, D>) {
        return true;
      }
      else {
        if (auto d = dynamic_cast<D*>(&data)) {
          user_module.process(*d);
          return true;
        }
        return false;
      }
    }

    void
    do_process(stage const st, node& data) final
    {
      switch (st) {
        case stage::setup:
          (setup<Ds>(data) || ...);
          return;
        case stage::process:
          (process<Ds>(data) || ...);
          return;
      }
    }

    T user_module;
  };
}

#endif /* sand_core_module_owner_hpp */
