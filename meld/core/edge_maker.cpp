#include "meld/core/edge_maker.hpp"

auto meld::edge_maker::maybe_graph_files(std::string const& file_prefix)
  -> std::unique_ptr<dot_files>
{
  if (file_prefix.empty()) {
    return nullptr;
  }
  return std::make_unique<dot_files>(dot_files{std::ofstream{file_prefix + "-functions.gv"},
                                               std::ofstream{file_prefix + "-data-pre.gv"},
                                               std::ofstream{file_prefix + "-data-post.gv"}});
}
