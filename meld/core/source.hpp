#ifndef meld_core_source_hpp
#define meld_core_source_hpp

#include "meld/core/source_owner.hpp"

#include "boost/dll/alias.hpp"

#define MELD_REGISTER_SOURCE(user_source)                                                          \
  BOOST_DLL_ALIAS(meld::source_owner<user_source>::create, create_source)

#endif /* meld_core_source_hpp */
