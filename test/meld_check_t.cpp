#include "meld/core/data_processor.hpp"
#include "meld/core/module_manager.hpp"
#include "test/test_module_multiple_levels.hpp"
#include "test/test_source.hpp"

#include "catch2/catch.hpp"

TEST_CASE("Module called", "[module]")
{
  using namespace meld;
  using module = test::multiple_levels;
  using source = test::my_source;

  boost::json::object const config{{"num_nodes", 5}};
  auto manager = module_manager::make_with_source<source>(config);
  manager.add_module<module, run, subrun>("module");

  meld::data_processor processor{&manager};
  processor.run_to_completion();

  std::vector const expected_runs{"1"_id, "3"_id, "5"_id};
  std::vector const expected_subruns{"1:2"_id, "3:4"_id};
  auto const& src = manager.get_source<source>();
  CHECK(src.created_runs == expected_runs);
  CHECK(src.created_subruns == expected_subruns);

  auto const& mod = manager.get_module<module, run, subrun>("module");
  transitions const expected_transitions{{"1"_id, stage::setup},
                                         {"1:2"_id, stage::process},
                                         {"1"_id, stage::process},
                                         {"3"_id, stage::setup},
                                         {"3:4:"_id, stage::process},
                                         {"3"_id, stage::process},
                                         {"5"_id, stage::setup},
                                         {"5"_id, stage::process}};
  CHECK(mod.processed_transitions == expected_transitions);
}
