#ifndef meld_model_qualified_name_hpp
#define meld_model_qualified_name_hpp

#include <span>
#include <string>

namespace meld {
  class qualified_name {
  public:
    qualified_name();
    qualified_name(char const* name);
    qualified_name(std::string name);
    qualified_name(std::string module, std::string name);

    std::string full(std::string const& delimiter = ":") const;
    std::string const& module() const noexcept { return module_; }
    std::string const& name() const noexcept { return name_; }

    bool operator==(qualified_name const& other) const;
    bool operator!=(qualified_name const& other) const;
    bool operator<(qualified_name const& other) const;

  private:
    std::string module_;
    std::string name_;
  };

  using qualified_names = std::span<qualified_name const, std::dynamic_extent>;

  class to_qualified_name {
  public:
    explicit to_qualified_name(std::string module) : module_{std::move(module)} {}
    qualified_name operator()(std::string const& name) const
    {
      return qualified_name{module_, name};
    }

  private:
    std::string module_;
  };
}

#endif /* meld_model_qualified_name_hpp */
