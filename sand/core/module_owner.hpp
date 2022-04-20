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
  constexpr std::string_view has_process_for = has_process<T, D> ? D::name() : "";

  template <typename T, typename D>
  concept has_setup = requires(T t, D const& d)
  {
    // clang-format off
    {t.setup(d)} -> std::same_as<void>;
    // clang-format on
  };
  template <typename T, typename D>
  constexpr std::string_view has_setup_for = has_setup<T, D> ? D::name() : "";

  template <typename T, typename... Ds>
  class module_owner : public module_worker {
  public:
    template <with_config U = T>
    explicit module_owner(boost::json::object const& config) : user_module{config}
    {
      if (auto deps = config.if_contains("dependencies")) {
        dependencies = value_to<std::vector<std::string>>(*deps);
      }
    }

    template <without_config U = T>
    explicit module_owner(boost::json::object const& config) : user_module{}
    {
      if (auto deps = config.if_contains("dependencies")) {
        dependencies = value_to<std::vector<std::string>>(*deps);
      }
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
    std::vector<std::string>
    required_dependencies() const final
    {
      return dependencies;
    }

    std::vector<transition_type>
    supported_setup_transitions() const final
    {
      std::vector transitions_with_setup{has_setup_for<T, Ds>...};
      transitions_with_setup.erase(
        std::remove(begin(transitions_with_setup), end(transitions_with_setup), ""),
        end(transitions_with_setup));
      std::vector<transition_type> result;
      std::transform(begin(transitions_with_setup),
                     end(transitions_with_setup),
                     back_inserter(result),
                     [](auto sv) {
                       return transition_type{sv, stage::setup};
                     });
      return result;
    }

    std::vector<transition_type>
    supported_process_transitions() const final
    {
      std::vector transitions_with_process{has_process_for<T, Ds>...};
      transitions_with_process.erase(
        std::remove(begin(transitions_with_process), end(transitions_with_process), ""),
        end(transitions_with_process));
      std::vector<transition_type> result;
      std::transform(begin(transitions_with_process),
                     end(transitions_with_process),
                     back_inserter(result),
                     [](auto sv) {
                       return transition_type{sv, stage::process};
                     });
      return result;
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

    T user_module;
    std::vector<std::string> dependencies;
  };
}

#endif /* sand_core_module_owner_hpp */
