#include "sand/run_sand.hpp"
#include "sand/core/data_processor.hpp"
#include "sand/core/load_module.hpp"

#include <iostream>

namespace sand {
  void
  run_it(boost::json::value const& configurations)
  {
    std::cout << "Running sand\n";
    auto const module_configs = configurations.at("modules").as_object();
    std::vector<module_worker_ptr> modules;
    for (auto const& [key, value] : module_configs) {
      modules.push_back(load_module(value.as_object()));
    }
    data_processor processor{load_source(configurations.at("source").as_object()), move(modules)};
    processor.run_to_completion();
  }
}
