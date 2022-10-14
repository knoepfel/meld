#include "meld/core/filter/filter_impl.hpp"

#include <cassert>

namespace meld {
  decision_map::decision_map(unsigned int total_decisions) : total_decisions_{total_decisions} {}

  void decision_map::update(filter_result result)
  {
    decltype(results_)::accessor a;
    results_.insert(a, result.msg_id);

    // First check that the decision is not already complete
    if (is_complete(a->second)) {
      return;
    }

    if (not result.result) {
      a->second = false_value;
      return;
    }

    ++a->second;

    if (a->second == total_decisions_) {
      a->second = true_value;
    }
  }

  unsigned int decision_map::value(std::size_t const msg_id) const
  {
    decltype(results_)::const_accessor a;
    if (results_.find(a, msg_id)) {
      return a->second;
    }
    return 0u;
  }

  void decision_map::erase(std::size_t const msg_id) { results_.erase(msg_id); }

  data_map::data_map(unsigned int const nargs) : nargs_{nargs} {}

  void data_map::update(std::size_t const msg_id, product_store_ptr const& store)
  {
    decltype(stores_)::accessor a;
    if (stores_.insert(a, msg_id)) {
      a->second.reserve(nargs_);
    }
    a->second.push_back(store);
  }

  bool data_map::is_complete(std::size_t const msg_id) const
  {
    decltype(stores_)::const_accessor a;
    if (stores_.find(a, msg_id)) {
      return a->second.size() == nargs_;
    }
    return false;
  }

  std::vector<product_store_ptr> data_map::release_data(std::size_t const msg_id)
  {
    decltype(stores_)::accessor a;
    bool const rc [[maybe_unused]] = stores_.find(a, msg_id);
    assert(rc);
    return std::move(a->second);
  }

  void data_map::erase(std::size_t const msg_id) { stores_.erase(msg_id); }

}
