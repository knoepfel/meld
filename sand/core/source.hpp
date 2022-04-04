#ifndef sand_core_source_hpp
#define sand_core_source_hpp

#include "sand/core/source_owner.hpp"

#include "boost/dll/alias.hpp"

#define SAND_REGISTER_SOURCE(user_source)                                                          \
  BOOST_DLL_ALIAS(sand::source_owner<user_source>::create, create_source)

#endif /* sand_core_source_hpp */
