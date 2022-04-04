#include "sand/run_sand.hpp"
#include "sand/version.hpp"

#include "CLI/App.hpp"
#include "CLI/Config.hpp"
#include "CLI/Formatter.hpp"

#include <iostream>
#include <string>

using namespace std::string_literals;

namespace {
}

int
main(int argc, char* argv[])
{
  CLI::App app{"sand is a framework to explore processing DUNE data."};
  bool maybe_version{false};
  unsigned num_nodes{1};
  app.add_flag("--version", maybe_version, "Print version of sand ("s + sand::version() + ")"s);
  app.add_option("-n", num_nodes, "Number of nodes to process (default is 1)");
  CLI11_PARSE(app, argc, argv);

  if (maybe_version) {
    std::cout << "sand " << sand::version() << '\n';
    return 0;
  }

  sand::run_it(num_nodes);
}
