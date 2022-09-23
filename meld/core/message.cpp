#include "meld/core/message.hpp"
#include "meld/graph/transition.hpp"

namespace meld {

  std::size_t
  MessageHasher::operator()(message const& msg) const noexcept
  {
    return msg.id;
  }

  message const&
  more_derived(message const& a, message const& b)
  {
    if (a.store->id().depth() > b.store->id().depth()) {
      return a;
    }
    return b;
  }
}
