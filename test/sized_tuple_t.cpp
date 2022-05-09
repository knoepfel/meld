#include "meld/utilities/sized_tuple.hpp"

#include <concepts>

using namespace std;
using namespace meld;

int
main()
{
  static_assert(same_as<sized_tuple<int, 3>, tuple<int, int, int>>);
  static_assert(
    same_as<concatenated_tuples<tuple<int>, sized_tuple<double, 4>, tuple<float, float>>,
            tuple<int, double, double, double, double, float, float>>);
}
