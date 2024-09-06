#ifndef meld_metaprogramming_detail_basic_concepts_hpp
#define meld_metaprogramming_detail_basic_concepts_hpp

namespace meld::detail {
  template <typename T>
  concept has_call_operator = requires { &T::operator(); };
}

#endif // meld_metaprogramming_detail_basic_concepts_hpp
