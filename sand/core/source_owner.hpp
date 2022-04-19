#ifndef sand_core_source_owner_hpp
#define sand_core_source_owner_hpp

#include "sand/core/node.hpp"
#include "sand/core/source_worker.hpp"
#include "sand/core/uses_config.hpp"

#include <iostream>
#include <memory>

namespace sand {
  template <typename T>
  concept source = requires(T t)
  {
    // clang-format off
    {t.data()} -> std::same_as<std::shared_ptr<node>>;
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
      id_t id_to_process = data ? data->id() : id_t{};

      if (data) {
        nodes.emplace(id_to_process, data);
      }
      else {
        more_data = false;
      }

      auto transitions_to_process = transitions_between(last_processed_level, id_to_process);
      last_processed_level = id_to_process;

      transition_messages result;
      for (auto const& [id, stage] : transitions_to_process) {
        auto node = nodes.at(id);
        transition_type const ttype{node->level_name(), stage};
        switch (stage) {
          case stage::setup:
            // The source retains the node when the setup stage is executed
            result.emplace_back(ttype, nodes.at(id));
            continue;
          case stage::process:
            // The source releases the node whenever the process stage is executed
            result.emplace_back(ttype, move(nodes.extract(id).mapped()));
        }
      }
      return result;
    }

    T user_source;
    id_t last_processed_level{};
    bool more_data{true};
    std::map<id_t, std::shared_ptr<node>> nodes;
  };
}

#endif /* sand_core_source_owner_hpp */
