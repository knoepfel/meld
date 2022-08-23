#include "meld/core/declared_reduction.hpp"
#include "meld/core/product_store.hpp"
#include "meld/graph/transition.hpp"

namespace meld {

  declared_reduction::declared_reduction(std::string name,
                                         std::size_t concurrency,
                                         std::vector<std::string> input_keys,
                                         std::vector<std::string> output_keys) :
    name_{move(name)},
    concurrency_{concurrency},
    input_keys_{move(input_keys)},
    output_keys_{move(output_keys)}
  {
  }

  declared_reduction::~declared_reduction() = default;

  void
  declared_reduction::invoke(product_store const& store)
  {
    if (store.is_flush()) {
      set_flush_value(store.id());
      return;
    }

    invoke_(store);
    auto& parent_id = store.parent()->id();
    auto it = entries_.find(parent_id);
    if (it == cend(entries_)) {
      it = entries_.emplace(parent_id, std::make_unique<map_entry>()).first;
    }
    ++it->second->count;
  }

  std::string const&
  declared_reduction::name() const noexcept
  {
    return name_;
  }

  std::size_t
  declared_reduction::concurrency() const noexcept
  {
    return concurrency_;
  }

  std::vector<std::string> const&
  declared_reduction::input() const noexcept
  {
    return input_keys_;
  }

  std::vector<std::string> const&
  declared_reduction::output() const noexcept
  {
    return output_keys_;
  }

  bool
  declared_reduction::reduction_complete(product_store& parent_store)
  {
    auto& entry = entries_.find(parent_store.id())->second;
    if (entry->count == entry->stop_after) {
      commit_(parent_store);
      // Would be good to free up memory here.
      entry.reset();
      return true;
    }
    return false;
  }

  void
  declared_reduction::set_flush_value(level_id const& id)
  {
    auto it = entries_.find(id.parent());
    assert(it != cend(entries_));
    it->second->stop_after = id.back();
  }
}
