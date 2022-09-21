#include "meld/core/declared_splitter.hpp"
#include "meld/core/handle.hpp"

namespace meld {

  generator::generator(product_store_ptr const& parent,
                       multiplexer& m,
                       std::atomic<std::size_t>& counter) :
    parent_{parent}, multiplexer_{m}, counter_{counter}
  {
  }

  void
  generator::make_child(std::size_t const i, products new_products)
  {
    ++counter_;
    ++calls_;
    auto child = parent_->make_child(i, std::move(new_products));
    multiplexer_.try_put({child, counter_});
  }

  message
  generator::flush_message()
  {
    auto const message_id = ++counter_;
    return {parent_->make_child(calls_, stage::flush), message_id, original_message_id_};
  }

  declared_splitter::declared_splitter(std::string name, std::size_t concurrency) :
    name_{move(name)}, concurrency_{concurrency}
  {
  }

  declared_splitter::~declared_splitter() = default;

  std::string const&
  declared_splitter::name() const noexcept
  {
    return name_;
  }

  std::size_t
  declared_splitter::concurrency() const noexcept
  {
    return concurrency_;
  }

  tbb::flow::receiver<message>&
  declared_splitter::port(std::string const& product_name)
  {
    return port_for(product_name);
  }
}
