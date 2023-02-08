#include "meld/core/declared_splitter.hpp"
#include "meld/model/handle.hpp"
#include "meld/model/level_counter.hpp"

namespace meld {

  generator::generator(product_store_const_ptr const& parent,
                       std::string const& node_name,
                       multiplexer& m,
                       tbb::flow::broadcast_node<message>& to_output,
                       std::atomic<std::size_t>& counter) :
    parent_{std::const_pointer_cast<product_store>(parent)},
    node_name_{node_name},
    multiplexer_{m},
    to_output_{to_output},
    counter_{counter}
  {
  }

  void generator::make_child(std::size_t const i,
                             std::string const& new_level_name,
                             products new_products)
  {
    ++child_counts_[new_level_name];
    ++counter_;
    message const msg{parent_->make_child(i, new_level_name, node_name_, std::move(new_products)),
                      counter_};
    to_output_.try_put(msg);
    multiplexer_.try_put(msg);
  }

  message generator::flush_message()
  {
    auto const message_id = ++counter_;
    auto flush_store = parent_->make_flush();
    if (not child_counts_.empty()) {
      flush_store->add_product(
        "[flush]",
        std::make_shared<flush_counts const>(parent_->id()->level_name(), move(child_counts_)));
    }
    return {move(flush_store), message_id, original_message_id_};
  }

  declared_splitter::declared_splitter(std::string name,
                                       std::vector<std::string> preceding_filters,
                                       std::vector<std::string> receive_stores) :
    products_consumer{move(name), move(preceding_filters), move(receive_stores)}
  {
  }

  declared_splitter::~declared_splitter() = default;
}
