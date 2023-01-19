# Comparison to art

Imagine the user has developed a piece of code that adds two numbers:

```C++
int add(int i, int j) { return i + j; }
```

Now imagine that the user would like to incorporate the `add` function into a framework
program.  In order for this to succeed, the user must tell the framework:

- What function to call
- When to call it
- Where to find the data that will serve as input arguments to the function
- How concurrent it can be


```C++
#include "art/Framework/Core/SharedProducer.h"
#include "add.hpp"

class Adder : public art::SharedProducer {
public:
  Adder(ParameterSet const&, art::ProcessingFrame const&)
  {
    produces<int>("sum");
    async<art::Event>();
  }

  void produce(art::Event& e, art::ProcessingFrame const&) override
  {
    auto const sum = add(e.getProduct<int>("i"), e.getProduct<int>("j"));
    e.put(std::make_unique<int>(sum), "sum");
  }
};

DEFINE_ART_MODULE(Adder);
```

```C++
#include "meld/module.hpp"
#include "add.hpp"

DEFINE_MODULE(m) {
  m.declare_transform(add).concurrency(unlimited).input("i", "j").output("sum");
}
```
