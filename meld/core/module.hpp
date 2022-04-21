#ifndef meld_core_module_hpp
#define meld_core_module_hpp

#include "meld/core/module_owner.hpp"

#include "boost/dll/alias.hpp"

#define MELD_REGISTER_MODULE(user_module, ...)                                                     \
  BOOST_DLL_ALIAS((meld::module_owner<user_module, __VA_ARGS__>::create), create_module)

#endif /* meld_core_module_hpp */
