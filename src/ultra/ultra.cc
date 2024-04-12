#include <ultra240/ultra.h>
#include "ultra/ultra.h"

namespace ultra {

  void init(const std::string& name) {
    path_manager::init(name);
    dynamic_library::init();
    renderer::init();
  }

  void quit() {
    renderer::quit();
    dynamic_library::quit();
  }

}
