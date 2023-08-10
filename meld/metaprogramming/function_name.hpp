#ifndef meld_metaprogramming_function_name_hpp
#define meld_metaprogramming_function_name_hpp

#include <string>

// =======================================================================================
// What's all this about?  To simplify function registration for users, I want it to be
// possible for users to type something like (e.g.):
//
//   m.with(my_transform).transform(...).to(...)
//
// instead of the more verbose:
//
//   m.with("my_transform", my_transform).transform(...).to(...)
//
// For the former case, the framework should be able to deduce that the name of the
// function is 'my_transform'.  Broadly speaking, this requires static reflection, which
// is not strongly supported by standard C++ (up through C++23).  However, Boost's
// stacktrace library does provide facilities to convert (free) function pointers to a
// "frame", which provides function-name information.
//
// This works very well for free functions; however, because non-static member function
// pointers do not mean much without a bound object, one has to resort to tricks (okay,
// hacks) to fool the compiler into treating the member-function pointer as a free
// function.  Doing this actually results in undefined behavior...however, it "seems to
// work" [cringe] for simply getting the names of the member functions.
//
// I think I should take a shower now.
// =======================================================================================

namespace meld {
  namespace detail {
    template <typename R, typename... Args>
    auto as_free_function(R (*f)(Args...))
    {
      return f;
    }

    // The remaining function templates are for coercing non-static member function
    // pointers into free functions.
    template <typename R, typename T, typename... Args>
    auto as_free_function(R (T::*f)(Args...))
    {
      return reinterpret_cast<R (*&)(Args...)>(f);
    }

    template <typename R, typename T, typename... Args>
    auto as_free_function(R (T::*f)(Args...) const)
    {
      return reinterpret_cast<R (*&)(Args...)>(f);
    }

    // noexcept-qualified versions
    template <typename R, typename T, typename... Args>
    auto as_free_function(R (T::*f)(Args...) noexcept)
    {
      return reinterpret_cast<R (*&)(Args...)>(f);
    }

    template <typename R, typename T, typename... Args>
    auto as_free_function(R (T::*f)(Args...) const noexcept)
    {
      return reinterpret_cast<R (*&)(Args...)>(f);
    }

    std::string stripped_name(std::string full_name);
    std::string stripped_name(void const* ptr);
  }

  std::string function_name(auto f)
  {
    using namespace detail;
    return stripped_name((void const*)as_free_function(f));
  }
}

#endif /* meld_metaprogramming_function_name_hpp */
