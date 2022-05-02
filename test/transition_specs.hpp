#ifndef test_transition_specs_hpp
#define test_transition_specs_hpp

#include "meld/core/transition.hpp"

namespace meld::test {
  inline transition
  setup(char const* spec)
  {
    return {id_for(spec), stage::setup};
  }
  inline transition
  flush(char const* spec)
  {
    return {id_for(spec), stage::flush};
  }
  inline transition
  process(char const* spec)
  {
    return {id_for(spec), stage::process};
  }
}

#endif /* test_transition_specs_hpp */
