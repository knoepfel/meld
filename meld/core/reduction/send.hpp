#ifndef meld_core_reduction_send_hpp
#define meld_core_reduction_send_hpp

// =======================================================================================
// Meld requires reduction results to be "sendable", where the result can be represented
// as a data product that is at least moveable.  For some types, notably std::atomic<T>
// specializations, move operations are not supported.  The framework thus permits a
// translation step where a non-moveable type can be converted to a type that is moveable.
//
// For non-moveable types, an overload of 'T send(U const&)' is required, where U is the
// unmoveable type, and T is the type that can be a represented as a data product.
// =======================================================================================

#include <atomic>
#include <concepts>

namespace meld {
  template <typename T>
  T send(std::atomic<T> const& a)
  {
    return a.load();
  }
}

#endif // meld_core_reduction_send_hpp
