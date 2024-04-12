#include <limits.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>
#include "ultra/ultra.h"

namespace ultra::path_manager {

  std::string data_dir;
  std::string lib_dir;

  static std::string get_bin_dir() {
    struct stat sb;
    if (lstat("/proc/self/exe", &sb) == -1) {
      throw error(__FILE__, __LINE__, "lstat error");
    }
    if (sb.st_size == 0) {
      sb.st_size = PATH_MAX;
    }
    char exe[sb.st_size + 1];
    if (readlink("/proc/self/exe", exe, sb.st_size + 1) < 0) {
      throw error(__FILE__, __LINE__, "readlink error");
    }
    exe[sb.st_size] = '\0';
    std::string sexe(exe);
    size_t last_slash_idx = sexe.rfind('/');
    if (last_slash_idx != std::string::npos) {
      sexe = sexe.substr(0, last_slash_idx);
    }
    return sexe;
  }

  void init(const std::string& name) {
    auto bin_dir = get_bin_dir();
    auto prefix = bin_dir.substr(0, bin_dir.rfind('/'));
    data_dir = prefix + "/share/" + name;
    lib_dir = prefix + "/lib/" + name;
  }

}
