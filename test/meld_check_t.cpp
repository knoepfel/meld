#include "meld/core/data_processor.hpp"
#include "meld/core/module_manager.hpp"
#include "test/test_module_multiple_levels.hpp"
#include "test/test_source.hpp"
#include "test/transition_specs.hpp"

#include "catch2/catch.hpp"

using namespace meld;
using namespace meld::test;

TEST_CASE("Module called", "[module]")
{
  using module = multiple_levels;
  using source = my_source;

  boost::json::object const config{{"num_nodes", 5}};
  auto manager = module_manager::make_with_source<source>(config);
  manager.add_module<module, run, subrun>("module");

  data_processor processor{&manager};
  processor.run_to_completion();

  std::vector const expected_runs{"1"_id, "3"_id, "5"_id};
  std::vector const expected_subruns{"1:2"_id, "3:4"_id};
  auto const& src = manager.get_source<source>();
  CHECK(src.created_runs == expected_runs);
  CHECK(src.created_subruns == expected_subruns);

  auto const& mod = manager.get_module<module, run, subrun>("module");
  auto const& processed_trs = mod.processed_transitions;

  transitions const expected_transitions{setup("1"),
                                         process("1:2"),
                                         process("1"),
                                         setup("3"),
                                         process("3:4"),
                                         process("3"),
                                         setup("5"),
                                         process("5")};
  auto it_for = [&processed_trs](transition const& tr) {
    return std::find(begin(processed_trs), end(processed_trs), tr);
  };

  // The order of execution for [1], [3], and [5] is irrelevant.
  // However, for each of thoses levels, the 'setup' must be first,
  // the processing of any sublevels must be second, and the 'process'
  // of the level must be last.
  CHECK(it_for(setup("1")) < it_for(process("1:2")));
  CHECK(it_for(process("1:2")) < it_for(process("1")));

  CHECK(it_for(setup("3")) < it_for(process("3:4")));
  CHECK(it_for(process("3:4")) < it_for(process("3")));

  CHECK(it_for(setup("5")) < it_for(process("5")));
}
