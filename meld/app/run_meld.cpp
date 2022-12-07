#include "meld/app/run_meld.hpp"
#include "meld/app/load_module.hpp"
#include "meld/concurrency.hpp"
#include "meld/core/framework_graph.hpp"

using namespace std::string_literals;

namespace meld {
  void run(boost::json::value const& configurations,
           std::optional<std::string> dot_file,
           int const max_parallelism)
  {
    framework_graph g{load_source(configurations.at("source").as_object()), max_parallelism};
    auto const module_configs = configurations.at("modules").as_object();
    for (auto const& [key, value] : module_configs) {
      load_module(g, value.as_object());
    }
    g.execute(dot_file.value_or(""s));
  }
}
