#include "sand/core/data_processor.hpp"
#include "sand/core/module_manager.hpp"

#include "catch2/catch.hpp"

namespace {
  struct source {
    std::shared_ptr<sand::node>
    data()
    {
      return nullptr;
    }
    unsigned nevents{10};
  };
}

TEST_CASE("Module called", "[module]")
{
  auto manager = sand::module_manager::make_with_source<source>();
  sand::data_processor processor{&manager};
  auto const& src = manager.get_source<source>();
  processor.run_to_completion();
  REQUIRE(src.nevents == 10);
}
