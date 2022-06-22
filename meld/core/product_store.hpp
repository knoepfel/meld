#ifndef meld_core_product_store_hpp
#define meld_core_product_store_hpp

#include "meld/utilities/debug.hpp"
#include "meld/utilities/demangle_symbol.hpp"

#include <map>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace meld {

  struct product_base {
    virtual ~product_base() = default;
  };

  template <typename T>
  struct product : product_base {
    explicit product(T const& prod) : obj{prod} {}
    std::remove_cvref_t<T> obj;
  };

  template <typename T>
  class handle {
  public:
    using value_type = T;
    handle() = default;
    explicit handle(product<T> const& prod) : t_{&prod.obj} {}
    T const*
    operator->() const noexcept
    {
      return t_;
    }
    T const&
    operator*() const noexcept
    {
      return *t_;
    }
    explicit operator bool() const noexcept { return t_ != nullptr; }
    operator T const&() const noexcept { return *t_; }
    operator T const*() const noexcept { return t_; }

  private:
    T const* t_{nullptr};
  };

  template <typename T>
  struct handle_ {
    using type = handle<std::remove_const_t<T>>;
  };

  template <typename T>
  struct handle_<T&> {
    static_assert(std::is_const_v<T>,
                  "If template argument to handle_for is a reference, it must be const.");
    using type = handle<std::remove_const_t<T>>;
  };

  template <typename T>
  struct handle_<T*> {
    static_assert(std::is_const_v<T>,
                  "If template argument to handle_for is a pointer, the pointee must be const.");
    using type = handle<std::remove_const_t<T>>;
  };

  template <typename T>
  struct handle_<handle<T>> {
    using type = handle<T>;
  };

  template <typename T>
  using handle_for = typename handle_<T>::type;

  class product_store {
  public:
    template <typename T>
    void add_product(std::string const& key, T const& t);

    template <typename T>
    handle<T> get_product(std::string const& key) const;

  private:
    std::map<std::string, std::shared_ptr<product_base>> products_;
  };

  // Implementation details
  template <typename T>
  void
  product_store::add_product(std::string const& key, T const& t)
  {
    debug("Adding ", demangle_symbol(typeid(std::remove_cvref_t<T>)));
    products_.try_emplace(key, std::make_shared<product<std::remove_cvref_t<T>>>(t));
  }

  template <typename T>
  handle<T>
  product_store::get_product(std::string const& key) const
  {
    auto& p = products_.at(key);
    if (auto t = std::dynamic_pointer_cast<product<T>>(p)) {
      return handle<T>{*t};
    }
    throw std::runtime_error("Cannot get product '" + key + "' with type " +
                             demangle_symbol(typeid(T)));
  }
}

#endif /* meld_core_product_store_hpp */
