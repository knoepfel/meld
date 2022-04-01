#include "sand/core/load_module.hpp"
#include "sand/core/module_worker.hpp"
#include "sand/core/source_worker.hpp"

#include "boost/dll/import.hpp"

#include <functional>

namespace sand {

  namespace {
    // If factory function goes out of scope, then the library is
    // unloaded...and that's bad.
    std::function<module_creator_t> create_module;
    std::function<source_creator_t> create_source;

    template <typename creator_t>
    auto
    plugin_loader(std::string const& spec, std::string const& symbol_name)
    {
      // FIXME: Remove hard-coding and choose policy for searching for plugins.
      std::filesystem::path shared_library_path("/Users/kyleknoepfel/work/build-sand/test");
      shared_library_path /= spec;

      using namespace boost::dll;
      return import_alias<creator_t>(
        shared_library_path, symbol_name, load_mode::append_decorations);
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
