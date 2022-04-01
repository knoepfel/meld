#ifndef sand_core_load_module_hpp
#define sand_core_load_module_hpp

#include "sand/core/module_worker.hpp"
#include "sand/core/source_worker.hpp"

#include <memory>
#include <string>

namespace sand {
  std::unique_ptr<module_worker> load_module(std::string const& spec);
  std::unique_ptr<source_worker> load_source(std::string const& spec, std::size_t n);
}

#endif
