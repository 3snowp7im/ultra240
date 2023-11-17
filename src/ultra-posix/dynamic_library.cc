#include <dlfcn.h>
#include <stdexcept>
#include <string>
#include "ultra.h"
#include "dynamic_library.h"

namespace ultra::dynamic_library {

  namespace posix {

    static std::string get_lib_path(const std::string& name) {
      std::string fname = name;
      if (fname.find('/') != std::string::npos) {
        std::string path = fname.substr(0, fname.rfind('/'));
        fname = path + "/lib" + fname.substr(fname.rfind('/') + 1);
      } else {
        fname = "lib" + fname;
      }
      return ultra::lib_dir + "/" + fname + ".so";
    }

    class DynamicLibrary : public ultra::dynamic_library::Impl::SystemImpl {
    public:

      static DynamicLibrary* deref(
        std::unique_ptr<ultra::dynamic_library::Impl::SystemImpl>& impl
      ) {
        return reinterpret_cast<DynamicLibrary*>(impl.get());
      }

      DynamicLibrary(const std::string& name)
        : handle(dlopen(get_lib_path(name).c_str(), RTLD_LAZY)) {
        if (handle == nullptr) {
          std::string dl_error(dlerror());
          throw std::runtime_error(
            "DynamicLibrary: could not open library: " + dl_error
          );
        }
      }

      ~DynamicLibrary() {
        dlclose(handle);
      }

      void* load_symbol(const std::string& name) {
        return dlsym(handle, name.c_str());
      }

      void* handle;
    };

  }

  void init() {}

  void quit() {}

  Impl::Impl(const std::string& name)
    : impl(new posix::DynamicLibrary(name)) {}

  Impl::~Impl() {
    close();
  }

  void Impl::close() {
    impl = nullptr;
  }

  void* Impl::load_symbol(const std::string& name) {
    auto impl = posix::DynamicLibrary::deref(this->impl);
    return impl->load_symbol(name);
  }

}
