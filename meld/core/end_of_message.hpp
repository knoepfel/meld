#ifndef meld_core_end_of_message_hpp
#define meld_core_end_of_message_hpp

#include "meld/core/fwd.hpp"
#include "meld/model/fwd.hpp"

#include <memory>

namespace meld {

  class end_of_message : public std::enable_shared_from_this<end_of_message> {
  public:
    static end_of_message_ptr make_base(level_hierarchy* hierarchy, level_id_ptr id);
    end_of_message_ptr make_child(level_id_ptr id);
    ~end_of_message();

  private:
    end_of_message(end_of_message_ptr parent, level_hierarchy* hierarchy, level_id_ptr id);

    end_of_message_ptr parent_;
    level_hierarchy* hierarchy_;
    level_id_ptr id_;
  };

}

#endif /* meld_core_end_of_message_hpp */
