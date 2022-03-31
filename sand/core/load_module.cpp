#include "sand/core/load_module.hpp"
#include "sand/core/module_worker.hpp"

#include "boost/dll/import.hpp"

#include <functional>

namespace sand {

  namespace {
    // If factory function goes out of scope, then the library is
    // unloaded...and that's bad.
    std::function<module_creator_t> create_module;
  }

  std::unique_ptr<module_worker>
  load_module()
  {
    boost::dll::fs::path shared_library_path("/Users/kyleknoepfel/work/build-sand/test");
    shared_library_path /= "test_module";

    create_module = boost::dll::import_alias<module_creator_t>(
      shared_library_path, "create_module", boost::dll::load_mode::append_decorations);

    return create_module();
  }
}
