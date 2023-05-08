#ifndef meld_core_registrar_hpp
#define meld_core_registrar_hpp

// =======================================================================================
//
// The registrar class completes the registration of a node at the end of a registration
// statement.  For example:
//
//   g.make<MyTransform>()
//     .declare_transform("name", &MyTransform::transform)
//     .concurrency(n)
//     .filtered_by(...)
//     .input(...)
//     .output(...);
//                 ^ Registration happens at the completion of the full statement.
//
// This is achieved by creating a class registrar class object (internally during any of
// the declare* calls), which is then passed along through each successive function call
// (concurrency, filtered_by, etc.).  When the statement completes (i.e. the semicolon is
// reached), the registrar object is destroyed, where the registrar's destructor registers
// the declared function as a graph node to be used by the framework.
//
// Timing
// ======
//
//    "Hurry.  Careful timing will we will need."  -Yoda (Star Wars, Episode III)
//
// In order for this system to work correctly, any intermediate objects created during the
// function-call chain above should contain the registrar object as its *last* data
// member.  This is to ensure that the registration happens before the rest of the
// intermediate object is destroyed, which could invalidate some of the data required
// during the registration process.
//
// Design rationale
// ================
//
// Consider the case of two output nodes:
//
//   g.make<MyOutput>().declare_output("all_slow", &MyOutput::output);
//   g.make<MyOutput>().declare_output("all_fast", &MyOutput::output).concurrency(n);
//   g.make<MyOutput>().declare_output("some_slow", &MyOutput::output).filtered_by(...);
//
// Each of the above registration statements are valid, but how the functions are
// registered with the framework depends on the function call-chain.  If the registration
// were to occur during the declare_output call, then it would be difficult to propagate
// the "concurrency" or "filtered_by" values.  By using the registrar class, we ensure
// that the user functions are registered at the end of each statement, after all the
// information has been specified.
//
// =======================================================================================

#include <functional>
#include <string>
#include <vector>

namespace meld {

  template <typename T>
  concept map_like = requires { typename T::mapped_type; };

  template <map_like Nodes>
  class registrar {
    using Creator = std::function<typename Nodes::mapped_type()>;

  public:
    explicit registrar(Nodes& nodes, std::vector<std::string>& errors) :
      nodes_{nodes}, errors_{errors}
    {
    }

    registrar(registrar const&) = delete;
    registrar& operator=(registrar const&) = delete;

    registrar(registrar&&) = default;
    registrar& operator=(registrar&&) = default;

    void set(Creator creator) { creator_ = std::move(creator); }
    ~registrar() noexcept(false)
    {
      if (creator_) {
        auto ptr = creator_();
        auto name = ptr->name();
        auto [_, inserted] = nodes_.try_emplace(name, std::move(ptr));
        if (not inserted) {
          errors_.push_back(fmt::format("Node with name '{}' already exists", name));
        }
      }
    }

  private:
    Nodes& nodes_;
    std::vector<std::string>& errors_;
    Creator creator_{};
  };

}

#endif /* meld_core_registrar_hpp */
