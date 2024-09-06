#ifndef meld_core_message_sender_hpp
#define meld_core_message_sender_hpp

#include "meld/core/fwd.hpp"
#include "meld/core/message.hpp"
#include "meld/core/multiplexer.hpp"
#include "meld/model/fwd.hpp"

#include <map>
#include <stack>

namespace meld {

  class message_sender {
  public:
    explicit message_sender(level_hierarchy& hierarchy,
                            multiplexer& mplexer,
                            std::stack<end_of_message_ptr>& eoms);

    void send_flush(product_store_ptr store);
    message make_message(product_store_ptr store);

  private:
    std::size_t original_message_id(product_store_ptr const& store);

    level_hierarchy& hierarchy_;
    multiplexer& multiplexer_;
    std::stack<end_of_message_ptr>& eoms_;
    std::map<level_id_ptr, std::size_t> original_message_ids_;
    std::size_t calls_{};
  };

}

#endif // meld_core_message_sender_hpp
