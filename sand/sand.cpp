#include "sand/run_sand.hpp"
#include "sand/version.hpp"

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

#include <fstream>
#include <iostream>
#include <string>

using namespace std::string_literals;
using namespace boost;

namespace {
  std::variant<json::value, json::error_code>
  read_json(std::string const& config)
  {
    json::stream_parser p{{}, json::parse_options{.allow_comments = true}};
    json::error_code ec;
    std::ifstream config_file{config};
    if (!config_file) {
      throw std::runtime_error("Malformed configuration file: " + config + ".");
    }
    for (std::string line; getline(config_file, line);) {
      // getline removes the trailing '\n' character; we add it back
      // because boost::json requires it when supporting comments.
      line.push_back('\n');
      p.write(line, ec);
      if (ec)
        return ec;
    }
    p.finish(ec);
    if (ec)
      return ec;
    return p.release();
  }
}

int
main(int argc, char* argv[])
{
  CLI::App app{"sand is a framework to explore processing DUNE data."};
  bool maybe_version{false};
  // unsigned num_nodes{1};
  std::string config_file;
  app.add_flag("--version", maybe_version, "Print version of sand ("s + sand::version() + ")"s);
  // app.add_option("-n", num_nodes, "Number of nodes to process (default is 1)");
  app.add_option("--config", config_file, "Configuration file to use.")->required();
  CLI11_PARSE(app, argc, argv);

  if (maybe_version) {
    std::cout << "sand " << sand::version() << '\n';
    return 0;
  }

  std::cout << "Using configuration file: " << config_file << '\n';
  auto result = read_json(config_file);
  if (auto ecp = get_if<boost::json::error_code>(&result)) {
    std::cerr << ecp->what() << '\n';
    return 1;
  }

  // TODO: Load all required plugins into a manager, and then allow 'run_it' to use the plugin manager.

  sand::run_it(get<boost::json::value>(result));
}
