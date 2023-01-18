#include "meld/metaprogramming/function_name.hpp"

#include "boost/stacktrace/frame.hpp"

#include <regex>

namespace {
  std::regex const keep_up_to_paren_right_before_function_params{R"((.*\w)\(.*)"};
}

namespace meld::detail {
  std::string stripped_name(void const* ptr)
  {
    // Get full name
    auto result = boost::stacktrace::frame{ptr}.name();
    result = std::regex_replace(result, keep_up_to_paren_right_before_function_params, "$1");
    // Remove any upfront qualifiers
    return result.substr(result.find_last_of(":") + 1ull);
  }
}
