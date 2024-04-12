#pragma once

#include <string>

namespace ultra::path_manager {

  extern std::string data_dir;

  extern std::string lib_dir;

  void init(const std::string& name);

}
