#ifndef meld_core_concurrency_tags_hpp
#define meld_core_concurrency_tags_hpp

#include "meld/graph/transition.hpp"
#include "meld/utilities/string_literal.hpp"

#include <array>
#include <optional>
#include <span>
#include <string_view>
#include <tuple>

namespace meld::concurrency {

  // Concurrency tags that can be specified by the user in the process/setup functions.

  struct serial /* implementation-defined */;
  struct unlimited /* implementation-defined */;

  template <int N>
  struct max /* implementation-defined */;

  template <string_literal... Resources>
    requires are_unique<Resources...>
  struct serial_for /* implementation-defined */;

  // ===========================================================
  // Implementation details

  struct serial {
    constexpr explicit serial(bool const specified) : is_specified{specified} {}
    static constexpr int value = 1;
    bool is_specified;
    static constexpr std::span<std::string_view> names{};
  };
  struct unlimited {
    static constexpr bool is_specified = true;
    static constexpr int value = 0;
    static constexpr std::span<std::string_view> names{};
  };
  template <int N>
  struct max {
    static constexpr bool is_specified = true;
    static constexpr int value = N;
    static constexpr std::span<std::string_view> names{};
  };
  template <string_literal... Resources>
    requires are_unique<Resources...>
  struct serial_for
  {
    static constexpr std::array names_{static_cast<std::string_view>(Resources)...};
    static constexpr bool is_specified = true;
    static constexpr int value = 1;
    static constexpr std::span<std::string_view const> names{names_};
  };

  namespace detail {
    template <typename T, typename D>
    constexpr auto
    tag(void (T::*)(D const&))
    {
      return serial{false};
    }

    template <typename T, typename D>
    constexpr auto
    tag(void (T::*)(D const&) const)
    {
      return serial{false};
    }

    template <typename T, typename D>
    constexpr auto
    tag(void (T::*)(D const&, serial))
    {
      return serial{true};
    }

    template <typename T, typename D>
    constexpr auto
    tag(void (T::*)(D const&, serial) const)
    {
      return serial{true};
    }

    template <typename T, typename D>
    constexpr auto
    tag(void (T::*)(D const&, unlimited))
    {
      return unlimited{};
    }

    template <typename T, typename D>
    constexpr auto
    tag(void (T::*)(D const&, unlimited) const)
    {
      return unlimited{};
    }

    template <typename T, typename D, int N>
    constexpr auto
    tag(void (T::*)(D const&, max<N>))
    {
      return max<N>{};
    }

    template <typename T, typename D, int N>
    constexpr auto
    tag(void (T::*)(D const&, max<N>) const)
    {
      return max<N>{};
    }

    template <typename T, typename D, string_literal... Resources>
    constexpr auto
    tag(void (T::*)(D const&, serial_for<Resources...>))
    {
      return serial_for<Resources...>{};
    }

    template <typename T, typename D, string_literal... Resources>
    constexpr auto
    tag(void (T::*)(D const&, serial_for<Resources...>) const)
    {
      return serial_for<Resources...>{};
    }
  }
}

namespace meld {
  template <typename T, typename D>
  constexpr auto
  concurrency_tag_for_process()
  {
    return concurrency::detail::tag<T, D>(&T::process);
  }
  template <typename T, typename D>
  constexpr auto
  concurrency_tag_for_setup()
  {
    return concurrency::detail::tag<T, D>(&T::setup);
  }

  template <typename T, typename D>
  concept supports_process = requires
  {
    concurrency::detail::tag<T, D>(&T::process);
  };

  template <typename T, typename D>
  concept supports_setup = requires
  {
    concurrency::detail::tag<T, D>(&T::setup);
  };

  constexpr bool
  operator==(std::span<std::string_view const> a, std::span<std::string_view const> b)
  {
    return std::equal(begin(a), end(a), begin(b), end(b));
  }

  struct concurrency_values {
    std::string_view name;
    std::optional<int> value;
    std::span<std::string_view const> resource_names;
    constexpr bool operator==(concurrency_values const&) const = default;
  };

  template <typename D>
  struct level_concurrency : concurrency_values {
  };

  template <typename T, typename D>
  constexpr level_concurrency<D>
  concurrency_for_process()
  {
    return {D::name()};
  }

  template <typename T, typename D>
  requires supports_process<T, D>
  constexpr level_concurrency<D>
  concurrency_for_process()
  {
    auto const tag = concurrency_tag_for_process<T, D>();
    return {D::name(), tag.value, tag.names};
  }

  template <typename T, typename D>
  constexpr level_concurrency<D>
  concurrency_for_setup()
  {
    return {D::name()};
  }

  template <typename T, typename D>
  requires supports_setup<T, D>
  constexpr level_concurrency<D>
  concurrency_for_setup()
  {
    auto const tag = concurrency_tag_for_setup<T, D>();
    return {D::name(), tag.value, tag.names};
  }

  template <stage s, typename T, typename... Args>
  constexpr auto
  concurrencies_for()
  {
    static_assert(s != stage::flush);
    if constexpr (s == stage::process) {
      return std::make_tuple(concurrency_for_process<T, Args>()...);
    }
    else {
      return std::make_tuple(concurrency_for_setup<T, Args>()...);
    }
  }

  template <stage s, typename T, typename... Args>
  class concurrencies {
  public:
    template <typename D>
    constexpr concurrency_values
    get() const noexcept
    {
      return std::get<level_concurrency<D>>(concurrencies_);
    }

    constexpr concurrency_values
    get(std::size_t i) const noexcept
    {
      assert(i < sizeof...(Args));
      return get_element<0>(i);
    }

    template <typename D>
    constexpr bool
    supports_level() const noexcept
    {
      if constexpr (not std::disjunction_v<std::is_same<D, Args>...>) {
        return false;
      }
      else {
        return std::get<level_concurrency<D>>(concurrencies_).value.has_value();
      }
    }

    constexpr concurrency_values
    get(std::string const& level_name) const noexcept
    {
      return get_element<0>(level_name);
    }

  private:
    std::tuple<level_concurrency<Args>...> concurrencies_{concurrencies_for<s, T, Args...>()};

    template <std::size_t I>
    constexpr concurrency_values
    get_element(std::size_t const i) const
    {
      if (i == I) {
        return std::get<I>(concurrencies_);
      }
      return get_element<I + 1>(i);
    }

    template <>
    constexpr concurrency_values
    get_element<sizeof...(Args)>(std::size_t) const
    {
      return {};
    }

    template <std::size_t I>
    constexpr concurrency_values
    get_element(std::string const& level_name) const
    {
      auto const& concurrency = std::get<I>(concurrencies_);
      if (concurrency.name == level_name) {
        return concurrency;
      }
      return get_element<I + 1>(level_name);
    }

    template <>
    constexpr concurrency_values
    get_element<sizeof...(Args)>(std::string const&) const
    {
      return {};
    }
  };

}

#endif /* meld_core_concurrency_tags_hpp */
