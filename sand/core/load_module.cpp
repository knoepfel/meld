#include "sand/core/load_module.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/source_worker.hpp"

#include "boost/algorithm/string.hpp"
#include "boost/dll/import.hpp"

#include <functional>
#include <string>

using namespace std::string_literals;

namespace sand {

  namespace {
    // If factory function goes out of scope, then the library is
    // unloaded...and that's bad.
    std::function<module_creator_t> create_module;
    std::function<source_creator_t> create_source;

    template <typename creator_t>
    std::function<creator_t>
    plugin_loader(std::string const& spec, std::string const& symbol_name)
    {
      char const* plugin_path_ptr = std::getenv("SAND_PLUGIN_PATH");
      if (!plugin_path_ptr)
        throw std::runtime_error("SAND_PLUGIN_PATH has not been set.");

      using namespace boost;
      std::vector<std::string> subdirs;
      split(subdirs, plugin_path_ptr, is_any_of(":"));

      // FIXME: Need to test to ensure that first match wins.
      for (auto const& subdir : subdirs) {
        std::filesystem::path shared_library_path{subdir};
        shared_library_path /= "lib" + spec + ".so";
        if (exists(shared_library_path)) {
          return dll::import_alias<creator_t>(shared_library_path, symbol_name);
        }
      }

      throw std::runtime_error("Could not locate library with specification '"s + spec +
                               "' in any directories on SAND_PLUGIN_PATH."s);
    }
  }

  std::unique_ptr<module_worker>
  load_module(std::string const& spec)
  {
    create_module = plugin_loader<module_creator_t>(spec, "create_module");
    return create_module();
  }

  std::unique_ptr<source_worker>
  load_source(std::string const& spec, std::size_t const n)
  {
    create_source = plugin_loader<source_creator_t>(spec, "create_source");
    return create_source(n);
  }
}
