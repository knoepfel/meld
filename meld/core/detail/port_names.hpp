#ifndef meld_core_detail_port_names_hpp
#define meld_core_detail_port_names_hpp

#include "meld/core/input_arguments.hpp"
#include "meld/core/specified_label.hpp"

#include <cstdint>
#include <array>
#include <tuple>
#include <utility>

namespace meld::detail {
  template <typename InputArgs>
  auto port_names(InputArgs const& args)
  {
    constexpr auto N = std::tuple_size_v<InputArgs>;
    auto unpack = []<std::size_t... Is>(InputArgs const& inputs, std::index_sequence<Is...>) {
      return std::array{std::get<Is>(inputs).label...};
    };
    return unpack(args, std::make_index_sequence<N>{});
  }
}

#endif /* meld_core_detail_port_names_hpp */
