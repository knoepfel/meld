#include "meld/utilities/type_deduction.hpp"

using namespace meld;

namespace {
  int transform [[maybe_unused]] (double&);
  void monitor [[maybe_unused]] (int);
  void only_void_param [[maybe_unused]] (void);
  std::tuple<> still_no_output [[maybe_unused]] ();
  std::tuple<int, double> two_output_objects [[maybe_unused]] ();
}

int main()
{
  static_assert(detail::number_input_parameters<decltype(transform)> == 1);
  static_assert(detail::number_input_parameters<decltype(monitor)> == 1);
  static_assert(detail::number_input_parameters<decltype(only_void_param)> == 0);

  static_assert(detail::number_output_objects<decltype(transform)> == 1);
  static_assert(detail::number_output_objects<decltype(monitor)> == 0);
  static_assert(detail::number_output_objects<decltype(only_void_param)> == 0);
  static_assert(detail::number_output_objects<decltype(still_no_output)> == 0);
  static_assert(detail::number_output_objects<decltype(two_output_objects)> == 2);
}
