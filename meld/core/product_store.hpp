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

  namespace detail {
    template <typename T>
    using handle_type = std::remove_cvref_t<T>;

    template <typename T, typename U>
    concept same_handle_type = std::same_as<handle_type<T>, handle_type<U>>;
  }

  template <typename T>
  class handle {
    using err_t = std::string;

  public:
    using value_type = detail::handle_type<T>;
    using const_reference = value_type const&;
    using const_pointer = value_type const*;

    handle() = default;

    template <typename U>
    explicit handle(product<U> const& prod) requires detail::same_handle_type<T, U> :
      rep_{&prod.obj}
    {
    }

    explicit handle(std::string err_msg) : rep_{move(err_msg)} {}

    const_pointer
    operator->() const
    {
      if (auto const* err = get_if<err_t>(&rep_)) {
        throw std::runtime_error(*err);
      }
      return get<const_pointer>(rep_);
    }
    [[nodiscard]] const_reference
    operator*() const
    {
      return *operator->();
    }
    explicit operator bool() const noexcept { return get_if<const_pointer>(&rep_) != nullptr; }
    operator const_reference() const noexcept { return operator*(); }
    operator const_pointer() const noexcept { return operator->(); }

    template <typename U>
    friend class handle;

    template <typename U>
    bool
    operator==(handle<U> rhs) const noexcept requires detail::same_handle_type<T, U>
    {
      return rep_ == rhs.rep_;
    }

  private:
    std::variant<const_pointer, err_t> rep_{"Cannot dereference empty handle of type '" +
                                            demangle_symbol(typeid(T)) + "'."};
  };

  template <typename T>
  handle(product<T> const&) -> handle<T>;

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

  template <typename T>
  struct labeled_data {
    std::string label;
    T data;
  };

  class product_store {
  public:
    template <typename T>
    void add_product(std::string const& key, T const& t);

    template <typename T>
    void add_product(labeled_data<T>&& data);

    template <typename T>
    T const& get_product(std::string const& key) const;

    template <typename T>
    handle<T> get_handle(std::string const& key) const;

  private:
    std::map<std::string, std::shared_ptr<product_base>> products_;
  };

  using product_store_ptr = std::shared_ptr<product_store>;
  inline auto
  make_product_store()
  {
    return std::make_shared<product_store>();
  }

  // Implementation details
  template <typename T>
  void
  product_store::add_product(std::string const& key, T const& t)
  {
    products_.try_emplace(key, std::make_shared<product<std::remove_cvref_t<T>>>(t));
  }

  template <typename T>
  void
  product_store::add_product(labeled_data<T>&& ldata)
  {
    add_product(ldata.label, std::forward<T>(ldata.data));
  }

  template <typename T>
  [[nodiscard]] handle<T>
  product_store::get_handle(std::string const& key) const
  {
    auto it = products_.find(key);
    if (it == cend(products_)) {
      return handle<T>{"No product exists with the key '" + key + "'."};
    }
    if (auto t = std::dynamic_pointer_cast<product<T>>(it->second)) {
      return handle<T>{*t};
    }
    return handle<T>{"Cannot get product '" + key + "' with type '" + demangle_symbol(typeid(T)) +
                     "'."};
  }

  template <typename T>
  [[nodiscard]] T const&
  product_store::get_product(std::string const& key) const
  {
    return *get_handle<T>(key);
  }
}

#endif /* meld_core_product_store_hpp */
