#include "boost/program_options.hpp"
#include "meld/app/run_meld.hpp"
#include "meld/app/version.hpp"

#include <fstream>
#include <iostream>
#include <string>

using namespace std::string_literals;
using namespace boost;
namespace bpo = boost::program_options;

namespace {
  std::variant<json::value, json::error_code> read_json(std::string const& config)
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

int main(int argc, char* argv[])
{
  std::ostringstream descstr;
  descstr << "\nUsage: " << std::filesystem::path(argv[0]).filename().native()
          << " -c <config-file> [other-options]\n\n"
          << "Basic options";
  bpo::options_description desc{
    descstr.str()}; //{"meld is a framework to explore processing DUNE data"};

  std::string config_file;
  // clang-format off
  desc.add_options()
    ("help,h", "Produce help message")
    ("version", ("Print meld version ("s + meld::version() + ")").c_str())
    ("config,c", bpo::value<std::string>(&config_file), "Configuration file.");
  // clang-format on

  // Parse the command line.
  bpo::variables_map vm;
  try {
    bpo::store(
      bpo::command_line_parser(argc, argv)
        .options(desc)
        .style(bpo::command_line_style::default_style & ~bpo::command_line_style::allow_guessing)
        .run(),
      vm);
    bpo::notify(vm);
  }
  catch (bpo::error const& e) {
    std::cerr << "Exception from command line processing in " << argv[0] << ": " << e.what()
              << '\n';
    return 1;
  }

  if (vm.count("help")) {
    std::cout << desc << '\n';
    return 0;
  }

  if (vm.count("version")) {
    std::cout << "meld " << meld::version() << '\n';
    return 0;
  }

  if (not vm.count("config")) {
    std::cerr << "Error: No configuration file given.\n";
    return 2;
  }

  std::cout << "Using configuration file: " << config_file << '\n';
  auto result = read_json(config_file);
  if (auto ecp = get_if<boost::json::error_code>(&result)) {
    std::cerr << ecp->what() << '\n';
    return 2;
  }

  meld::run_it(get<boost::json::value>(result));
}
