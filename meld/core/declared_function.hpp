#ifndef meld_core_declared_function_hpp
#define meld_core_declared_function_hpp

#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace meld {

  class product_store; // FIXME: Put this in fwd file

  class declared_function {
  public:
    declared_function(std::string name,
                      std::vector<std::string> input_keys,
                      std::vector<std::string> output_keys) :
      name_{move(name)}, input_keys_{move(input_keys)}, output_keys_{move(output_keys)}
    {
    }

    virtual ~declared_function() = default;

    void
    invoke(product_store& store) const
    {
      invoke_(store);
    }

    auto const&
    name() const noexcept
    {
      return name_;
    }

    auto const&
    input() const noexcept
    {
      return input_keys_;
    }
    auto const&
    output() const noexcept
    {
      return output_keys_;
    }

  private:
    virtual void invoke_(product_store& store) const = 0;

    std::string name_;
    std::vector<std::string> input_keys_;
    std::vector<std::string> output_keys_;
  };

  using declared_function_ptr = std::unique_ptr<declared_function>;
  using declared_functions = std::map<std::string, declared_function_ptr>;
}
#endif /* meld_core_declared_function_hpp */
