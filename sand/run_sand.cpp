#include "sand/run_sand.hpp"
#include "sand/core/data_processor.hpp"
#include "sand/core/load_module.hpp"
#include "sand/core/module_manager.hpp"

#include <iostream>

namespace sand {
  void
  run_it(boost::json::value const& configurations)
  {
    std::cout << "Running sand\n";
    auto const module_configs = configurations.at("modules").as_object();
    std::map<std::string, module_worker_ptr> modules;
    for (auto const& [key, value] : module_configs) {
      modules.emplace(std::string(key), load_module(value.as_object()));
    }
    module_manager manager{load_source(configurations.at("source").as_object()), move(modules)};
    data_processor processor{&manager};
    processor.run_to_completion();
  }
}
