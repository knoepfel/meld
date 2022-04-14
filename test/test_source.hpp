#ifndef test_test_source_hpp
#define test_test_source_hpp

#include "sand/core/source.hpp"
#include "test/data_levels.hpp"

#include <iostream>

namespace sand::test {
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
          run_ = null_node.make_child<run>(cursor_);
          std::cout << "Creating run " << *run_ << '\n';
          created_runs.push_back(run_->id());
          return run_;
        }
        auto srun = run_->make_child<subrun>(cursor_);
        std::cout << "Creating subrun " << *srun << '\n';
        created_subruns.push_back(srun->id());
        return srun;
      }
      return nullptr;
    }

    // For testing
    std::vector<id_t> created_runs;
    std::vector<id_t> created_subruns;

  private:
    std::size_t const num_nodes_;
    std::shared_ptr<run> run_;
    std::size_t cursor_{0};
  };

}

#endif /* test_test_source_hpp */
