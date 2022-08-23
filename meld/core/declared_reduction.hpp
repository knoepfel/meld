#ifndef meld_core_declared_reduction_hpp
#define meld_core_declared_reduction_hpp

#include "meld/graph/transition.hpp"

#include "oneapi/tbb/concurrent_unordered_map.h"

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace meld {

  class product_store; // FIXME: Put this in fwd file

  class declared_reduction {
  public:
    declared_reduction(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> input_keys,
                       std::vector<std::string> output_keys);

    virtual ~declared_reduction();

    void invoke(product_store const& store);
    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    std::vector<std::string> const& input() const noexcept;
    std::vector<std::string> const& output() const noexcept;

    bool reduction_complete(product_store& parent_store);

  private:
    virtual void invoke_(product_store const& store) = 0;
    virtual void commit_(product_store& store) = 0;

    void set_flush_value(level_id const& id);

    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> input_keys_;
    std::vector<std::string> output_keys_;
    struct map_entry {
      std::atomic<unsigned int> count{};
      std::atomic<unsigned int> stop_after{-1u};
    };
    tbb::concurrent_unordered_map<level_id, std::unique_ptr<map_entry>> entries_;
  };

  using declared_reduction_ptr = std::unique_ptr<declared_reduction>;
  using declared_reductions = std::map<std::string, declared_reduction_ptr>;
}

#endif /* meld_core_declared_reduction_hpp */
