#include "meld/metaprogramming/function_name.hpp"

#include "boost/stacktrace/frame.hpp"

#include <regex>

namespace {
  std::regex const keep_up_to_paren_right_before_function_params{R"((.*\w)\(.*)"};
}

namespace meld::detail {
  std::string stripped_name(std::string full_name)
  {
    full_name = std::regex_replace(full_name, keep_up_to_paren_right_before_function_params, "$1");
    // Remove any upfront qualifiers
    return full_name.substr(full_name.find_last_of(":") + 1ull);
  }

  std::string stripped_name(void const* ptr)
  {
    return stripped_name(boost::stacktrace::frame{ptr}.name());
  }
}
