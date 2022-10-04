#include "meld/app/run_meld.hpp"
#include "meld/core/framework_graph.hpp"
#include "meld/core/load_module.hpp"

namespace meld {
  void run_it(boost::json::value const& configurations)
  {
    framework_graph g{load_source(configurations.at("source").as_object())};
    auto const module_configs = configurations.at("modules").as_object();
    for (auto const& [key, value] : module_configs) {
      load_module(g, value.as_object());
    }
    g.execute();
  }
}
