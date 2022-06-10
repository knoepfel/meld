#ifndef meld_utilities_make_edges_hpp
#define meld_utilities_make_edges_hpp

#include "meld/utilities/cartesian_product.hpp"

#include <cstddef>
#include <tuple>

namespace meld {
  namespace detail {
    template <typename FT, typename... T>
    class node_t;

    template <typename FT, typename... T>
    class edge_maker {
    public:
      template <typename... U>
      explicit edge_maker(FT const& ft, std::tuple<U...> nodes) : ft_{ft}, nodes_{nodes}
      {
      }
      template <typename... U>
      node_t<FT, U...> nodes(U... j) const;

    private:
      FT const& ft_;
      std::tuple<T...> nodes_;
    };

    template <typename FT, typename... T>
    class node_t {
    public:
      template <typename F, typename... U>
      explicit node_t(F const& f, U... i) : maker_{f, std::make_tuple(i...)}
      {
      }

      edge_maker<FT, T...> const*
      operator->() const
      {
        return &maker_;
      }

    private:
      edge_maker<FT, T...> maker_;
    };

    template <typename FT, typename... T>
    template <typename... U>
    node_t<FT, U...>
    edge_maker<FT, T...>::nodes(U... j) const
    {
      cartesian_product(nodes_, std::make_tuple(j...), ft_);
      return node_t<FT, U...>{ft_, j...};
    }
  }

  template <typename FT, typename... T>
  auto
  nodes_using(FT const& ft, T... t)
  {
    return detail::node_t<FT, T...>{ft, t...};
  }
}

#endif /* meld_utilities_make_edges_hpp */
