#ifndef sand_core_module_hpp
#define sand_core_module_hpp

#include "sand/core/module_owner.hpp"

#include "boost/dll/alias.hpp"

#define SAND_REGISTER_MODULE(user_module, ...)                                                     \
  BOOST_DLL_ALIAS((sand::module_owner<user_module, __VA_ARGS__>::create), create_module)

#endif
