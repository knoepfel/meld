#include "meld/core/framework_graph.hpp"
#include "meld/core/product_store.hpp"
#include "meld/utilities/indicators.hpp"
#include "test/products_for_output.hpp"

using namespace meld;

namespace {
  unsigned pass_on(unsigned number) { return number; }
}

int main()
{
  constexpr auto max_events{10'000u};

  using namespace indicators;

  show_console_cursor(false);
  ProgressBar bar{option::BarWidth{60},
                  option::ForegroundColor{Color::white},
                  option::PrefixText{"Processing " + std::to_string(max_events) + " events: "},
                  option::ShowPercentage{true},
                  option::FontStyles{std::vector<FontStyle>{FontStyle::bold}},
                  option::MaxProgress{max_events}};

  level_id const root_id{};
  framework_graph g{[&root_id, &bar, i = 0u]() mutable -> product_store_ptr {
    if (i == max_events) {
      return nullptr;
    }
    auto store = make_product_store(root_id.make_child(i));
    store->add_product("number", i++);

    bar.tick();
    return store;
  }};
  g.declare_transform("pass_on", pass_on).input("number").output("different");
  g.execute();

  show_console_cursor(true);
}
