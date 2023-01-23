#ifndef meld_module_hpp
#define meld_module_hpp

#include "boost/dll/alias.hpp"
#include "meld/concurrency.hpp"
#include "meld/configuration.hpp"
#include "meld/core/graph_proxy.hpp"

#include "boost/preprocessor.hpp"

namespace meld::detail {
  using module_creator_t = void(graph_proxy<void_tag>&, configuration const&);
}

#define NARGS(...) BOOST_PP_DEC(BOOST_PP_VARIADIC_SIZE(__VA_OPT__(, ) __VA_ARGS__))

#define CREATE_1ARG(m) void create(meld::graph_proxy<meld::void_tag>& m, meld::configuration const&)
#define CREATE_2ARGS(m, pset)                                                                      \
  void create(meld::graph_proxy<meld::void_tag>& m, meld::configuration const& config)

#define SELECT_SIGNATURE(...)                                                                      \
  BOOST_PP_IF(BOOST_PP_EQUAL(NARGS(__VA_ARGS__), 1), CREATE_1ARG, CREATE_2ARGS)(__VA_ARGS__)

#define DEFINE_MODULE(...)                                                                         \
  static SELECT_SIGNATURE(__VA_ARGS__);                                                            \
  BOOST_DLL_ALIAS(create, create_module)                                                           \
  SELECT_SIGNATURE(__VA_ARGS__)

#endif /* meld_module_hpp */
