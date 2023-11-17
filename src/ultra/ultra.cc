#include <ultra240/ultra.h>
#include "ultra.h"
#include "dynamic_library.h"
#include "path_manager.h"
#include "renderer.h"

namespace ultra {

  std::string data_dir;
  std::string lib_dir;

  void init(const std::string& name) {
    path_manager::init(name);
    data_dir = path_manager::data_dir;
    lib_dir = path_manager::lib_dir;
    dynamic_library::init();
    renderer::init();
  }

  void quit() {
    renderer::quit();
    dynamic_library::quit();
  }

}
