#ifndef test_test_source_hpp
#define test_test_source_hpp

#include "meld/core/source.hpp"
#include "meld/utilities/debug.hpp"
#include "test/data_levels.hpp"

namespace meld::test {
  class my_source {
  public:
    explicit my_source(boost::json::object const& config) :
      num_nodes_{config.at("num_nodes").to_number<unsigned>()}
    {
    }

    std::shared_ptr<node>
    data()
    {
      if (cursor_ < num_nodes_) {
        ++cursor_;
        if (cursor_ % 2 != 0) {
          run_ = root_node->make_child<run>(cursor_);
          debug("Creating run ", *run_);
          created_runs.push_back(run_->id());
          return run_;
        }
        auto srun = run_->make_child<subrun>(cursor_);
        debug("Creating subrun ", *srun);
        created_subruns.push_back(srun->id());
        return srun;
      }
      return nullptr;
    }

    // For testing
    std::vector<level_id> created_runs;
    std::vector<level_id> created_subruns;

  private:
    std::size_t const num_nodes_;
    std::shared_ptr<run> run_;
    std::size_t cursor_{0};
  };

}

#endif /* test_test_source_hpp */
