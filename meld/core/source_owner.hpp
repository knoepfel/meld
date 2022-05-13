#ifndef meld_core_source_owner_hpp
#define meld_core_source_owner_hpp

#include "meld/core/source_worker.hpp"
#include "meld/core/uses_config.hpp"
#include "meld/graph/data_node.hpp"
#include "meld/utilities/debug.hpp"

#include <cassert>
#include <iostream>
#include <memory>

namespace meld {
  template <typename T>
  concept source = requires(T t)
  {
    // clang-format off
    {t.data()} -> std::same_as<std::shared_ptr<data_node>>;
    // clang-format on
  };

  template <source T>
  class source_owner : public source_worker {
  public:
    template <with_config U = T>
    explicit source_owner(boost::json::object const& config) : user_source{config}
    {
    }

    template <without_config U = T>
    explicit source_owner(boost::json::object const&) : user_source{}
    {
    }

    static std::unique_ptr<source_worker>
    create(boost::json::object const& config)
    {
      return std::make_unique<source_owner<T>>(config);
    }

    T const&
    source() const noexcept
    {
      return user_source;
    }

  private:
    transition_messages
    next_transitions() final
    {
      if (!more_data) {
        return {};
      }

      auto data = user_source.data();
      level_id id_to_process = data ? data->id() : level_id{};

      if (data) {
        nodes.emplace(id_to_process, data);
      }
      else {
        more_data = false;
      }

      auto transitions_to_process =
        transitions_between(last_processed_level, id_to_process, counter);

      // FIXME: This is icky
      if (last_processed_level == ""_id) {
        nodes.emplace(""_id, root_node);
        transitions_to_process.insert(begin(transitions_to_process),
                                      transition{""_id, stage::setup});
      }
      last_processed_level = id_to_process;

      transition_messages result;
      for (auto const& tr : transitions_to_process) {
        auto const& [id, stage] = tr;
        if (stage == stage::process) {
          // The source releases the node whenever the process stage is executed
          result.emplace_back(tr, move(nodes.extract(id).mapped()));
        }
        else if (stage == stage::setup) {
          // The source retains the node when the setup stage is executed
          result.emplace_back(tr, nodes.at(id));
        }
        else {
          assert(stage == stage::flush);
          result.emplace_back(tr, nullptr);
        }
      }
      return result;
    }

    T user_source;
    level_id last_processed_level{};
    bool more_data{true};
    std::map<level_id, std::shared_ptr<data_node>> nodes;
    level_counter counter;
  };
}

#endif /* meld_core_source_owner_hpp */
