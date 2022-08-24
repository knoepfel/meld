#ifndef meld_core_declared_transform_hpp
#define meld_core_declared_transform_hpp

#include <cstddef>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace meld {
  class product_store;

  class declared_transform {
  public:
    declared_transform(std::string name,
                       std::size_t concurrency,
                       std::vector<std::string> input_keys,
                       std::vector<std::string> output_keys);

    virtual ~declared_transform();

    void invoke(product_store& store) const;
    std::string const& name() const noexcept;
    std::size_t concurrency() const noexcept;
    std::vector<std::string> const& input() const noexcept;
    std::vector<std::string> const& output() const noexcept;

  private:
    virtual void invoke_(product_store& store) const = 0;

    std::string name_;
    std::size_t concurrency_;
    std::vector<std::string> input_keys_;
    std::vector<std::string> output_keys_;
  };

  using declared_transform_ptr = std::unique_ptr<declared_transform>;
  using declared_transforms = std::map<std::string, declared_transform_ptr>;
}

#endif /* meld_core_declared_transform_hpp */
