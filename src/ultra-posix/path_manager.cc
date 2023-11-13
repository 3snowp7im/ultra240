#include <climits>
#include <cstring>
#include <ultra240/file.h>
#include <ultra240/path_manager.h>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace ultra::posix {

  class PathManagerImpl : public PathManager::Impl {
  public:

    static PathManagerImpl* deref(
      std::unique_ptr<PathManager::Impl>& impl
    ) {
      return reinterpret_cast<PathManagerImpl*>(impl.get());
    }

    static const PathManagerImpl* deref(
      const std::unique_ptr<PathManager::Impl>& impl
    ) {
      return reinterpret_cast<const PathManagerImpl*>(impl.get());
    }

    static std::string get_bin_dir() {
      struct stat sb;
      if (lstat("/proc/self/exe", &sb) == -1) {
        throw std::runtime_error("PathManager: lstat error");
      }
      if (sb.st_size == 0) {
        sb.st_size = PATH_MAX;
      }
      char exe[sb.st_size + 1];
      if (readlink("/proc/self/exe", exe, sb.st_size + 1) < 0) {
        throw std::runtime_error("PathManager: readlink error");
      }
      exe[sb.st_size] = '\0';
      std::string directory;
      char* last_slash_idx = const_cast<char*>(strrchr(exe, '/'));
      if (last_slash_idx != nullptr) {
        *last_slash_idx = '\0';
      }
      return std::string(exe);
    }

    PathManagerImpl(const char* name)
      : bin_dir(get_bin_dir()),
        name(name) {}

    std::string get_user_path(const char* name) const {
      std::string sname(name == nullptr ? "" : name);
      std::string spath;
      if (sname.size()) {
        if (sname.rfind('/') != std::string::npos) {
          spath = "/" + sname.substr(0, sname.rfind('/') + 1);
          sname = sname.substr(sname.rfind('/') + 1);
        } else {
          spath = "/";
        }
      }
      return std::string(getenv("HOME"))
        + "/.config/"
        + this->name
        + spath
        + sname;
    }

    std::string get_data_path(const char* name) const {
      std::string sname(name == nullptr ? "" : name);
      std::string spath;
      if (sname.size()) {
        if (sname.rfind('/') != std::string::npos) {
          spath = "/" + sname.substr(0, sname.rfind('/') + 1);
          sname = sname.substr(sname.rfind('/') + 1);
        } else {
          spath = "/";
        }
      }
      return bin_dir
        + "/../share/"
        + this->name
        + spath
        + sname;
    }

    std::string get_lib_path(const char* name) const {
      std::string sname(name == nullptr ? "" : name);
      std::string spath;
      if (sname.size()) {
        if (sname.rfind('/') != std::string::npos) {
          spath = "/" + sname.substr(0, sname.rfind('/') + 1);
          sname = sname.substr(sname.rfind('/') + 1);
        } else {
          spath = "/";
        }
        sname = "lib" + sname + ".so";
      }
      return bin_dir
        + "/../lib/"
        + this->name
        + spath
        + sname;
    }

    std::string bin_dir;

    std::string name;
  };

}

namespace ultra {

  using namespace ultra::posix;

  void PathManager::init() {}

  void PathManager::quit() {}

  PathManager::PathManager(const char* name)
    : impl(new PathManagerImpl(name)) {}

  PathManager::~PathManager() {}

  std::string PathManager::get_user_path(const char* name) const {
    auto impl = PathManagerImpl::deref(this->impl);
    return impl->get_user_path(name);
  }

  std::string PathManager::get_data_path(const char* name) const {
    auto impl = PathManagerImpl::deref(this->impl);
    return impl->get_data_path(name);
  }

  std::string PathManager::get_lib_path(const char* name) const {
    auto impl = PathManagerImpl::deref(this->impl);
    return impl->get_lib_path(name);
  }

}
