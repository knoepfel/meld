#include "meld/utilities/sized_tuple.hpp"

#include <array>
#include <concepts>
#include <span>
#include <tuple>

using namespace std;
using namespace meld;

int main()
{
  static_assert(same_as<sized_tuple<int, 3>, tuple<int, int, int>>);
  static_assert(
    same_as<concatenated_tuples<tuple<int>, sized_tuple<double, 4>, tuple<float, float>>,
            tuple<int, double, double, double, double, float, float>>);
  constexpr static std::array nums{1, 2, 3, 4};
  constexpr static std::span<int const> view{nums};
  static_assert(sized_tuple_from<int const, 4>(view) == std::make_tuple(1, 2, 3, 4));
}
