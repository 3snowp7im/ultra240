#include <memory>
#include <ultra240/util.h>

namespace ultra::util {

  DynamicLibrary* load_library(ResourceLoader& loader, const char* name) {
    std::string path = "lib" + std::string(name) + ".so";
    return DynamicLibrary::create(loader.get_lib_path(path.c_str()).c_str());
  }

}
