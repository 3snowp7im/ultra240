#include <climits>
#include <cstring>
#include <ultra240/file.h>
#include <ultra240/resource_loader.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace ultra::posix {

  class ResourceLoader : public ultra::ResourceLoader {
    const std::string bin_dir;
    const std::string name;
  public:
    ResourceLoader(const char* name);
    std::string get_data_path(const char* path);
    std::string get_lib_path(const char* path);
    file::Input* open_data(const char* path);
    file::Input* open_lib(const char* path);
  };

  static std::string get_bin_dir() {
    struct stat sb;
    if (lstat("/proc/self/exe", &sb) == -1) {
      throw std::runtime_error("ResourceLoader: lstat error");
    }
    if (sb.st_size == 0) {
      sb.st_size = PATH_MAX;
    }
    char exe[sb.st_size + 1];
    if (readlink("/proc/self/exe", exe, sb.st_size + 1) < 0) {
      throw std::runtime_error("ResourceLoader: readlink error");
    }
    exe[sb.st_size] = '\0';
    std::string directory;
    char* last_slash_idx = const_cast<char*>(strrchr(exe, '/'));
    if (last_slash_idx != nullptr) {
      *last_slash_idx = '\0';
    }
    return std::string(exe);
  }

  ResourceLoader::ResourceLoader(const char* name)
    : bin_dir(get_bin_dir()),
      name(name) {
  }

  std::string ResourceLoader::get_data_path(const char* path) {
    return bin_dir + "/../share/" + name + "/" + path;
  }

  std::string ResourceLoader::get_lib_path(const char* path) {
    return bin_dir + "/../lib/" + name + "/" + path;
  }

  file::Input* ResourceLoader::open_data(const char* path) {
    return file::Input::open(get_data_path(path).c_str());
  }

  file::Input* ResourceLoader::open_lib(const char* path) {
    return file::Input::open(get_lib_path(path).c_str());
  }

}

namespace ultra {

  void ResourceLoader::init() {
  }

  void ResourceLoader::quit() {
  }

  ResourceLoader* ResourceLoader::create(const char* name) {
    return new posix::ResourceLoader(name);
  }

}
