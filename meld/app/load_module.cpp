#include "meld/app/load_module.hpp"
#include "meld/configuration.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/module.hpp"
#include "meld/source.hpp"

#include "boost/algorithm/string.hpp"
#include "boost/dll/import.hpp"
#include "boost/json.hpp"

#include <functional>
#include <string>

using namespace std::string_literals;

namespace meld {

  namespace {
    // If factory function goes out of scope, then the library is unloaded...and that's
    // bad.
    std::vector<std::function<detail::module_creator_t>> create_module;
    std::function<detail::source_creator_t> create_source;

    template <typename creator_t>
    std::function<creator_t> plugin_loader(std::string const& spec, std::string const& symbol_name)
    {
      char const* plugin_path_ptr = std::getenv("MELD_PLUGIN_PATH");
      if (!plugin_path_ptr)
        throw std::runtime_error("MELD_PLUGIN_PATH has not been set.");

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
                               "' in any directories on MELD_PLUGIN_PATH."s);
    }
  }

  void load_module(framework_graph& g, std::string const& label, boost::json::object raw_config)
  {
    auto const& spec = value_to<std::string>(raw_config.at("plugin"));
    auto& creator =
      create_module.emplace_back(plugin_loader<detail::module_creator_t>(spec, "create_module"));
    raw_config["module_label"] = label;

    configuration const config{raw_config};
    auto module_proxy = g.proxy(config);
    creator(module_proxy, config);
  }

  std::function<product_store_ptr()> load_source(boost::json::object const& config)
  {
    auto const& spec = value_to<std::string>(config.at("plugin"));
    create_source = plugin_loader<detail::source_creator_t>(spec, "create_source");
    return create_source(config);
  }
}
