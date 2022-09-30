#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"
// #include "meld/utilities/indicators.hpp"
#include "test/products_for_output.hpp"

using namespace meld;
using namespace meld::concurrency;

namespace {
  unsigned pass_on(unsigned number) { return number; }
}

int main()
{
  constexpr auto max_events{100'000u};

  // using namespace indicators;

  // show_console_cursor(false);
  // ProgressBar bar{option::BarWidth{60},
  //                 option::ForegroundColor{Color::cyan},
  //                 option::PrefixText{"Processing " + std::to_string(max_events) + " events: "},
  //                 option::ShowPercentage{true},
  //                 option::ShowElapsedTime{true},
  //                 option::MaxProgress{max_events * 2u}};

  level_id const root_id{};
  framework_graph g{[&root_id, /* &bar,*/ i = 0u]() mutable -> product_store_ptr {
    if (i == max_events * 2) {
      return nullptr;
    }
    auto [event_number, is_flush] = std::make_tuple(i / 2u, i % 2u != 0u);
    auto proto_id = root_id.make_child(event_number);
    if (is_flush) {
      proto_id = proto_id.make_child(0);
    }
    auto const stage = is_flush ? stage::flush : stage::process;
    auto store = make_product_store(proto_id, "Source", stage);
    if (not is_flush) {
      store->add_product("number", i);
    }
    ++i;
    // bar.tick();
    return store;
  }};
  g.declare_transform("pass_on", pass_on)
    .concurrency(unlimited)
    .input("number")
    .output("different");
  g.execute();

  // show_console_cursor(true);
}
