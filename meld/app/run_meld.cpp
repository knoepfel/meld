#include "meld/app/run_meld.hpp"
#include "meld/core/data_processor.hpp"
#include "meld/core/load_module.hpp"
#include "meld/core/module_manager.hpp"

#include <iostream>

namespace meld {
  void
  run_it(boost::json::value const& configurations)
  {
    std::cout << "Running meld\n";
    auto const module_configs = configurations.at("modules").as_object();
    std::map<std::string, module_worker_ptr> modules;
    for (auto const& [key, value] : module_configs) {
      modules.emplace(std::string(key), load_module(value.as_object()));
    }
    module_manager manager{load_source(configurations.at("source").as_object()), move(modules)};
    data_processor processor{manager};
    processor.run_to_completion();
  }
}
