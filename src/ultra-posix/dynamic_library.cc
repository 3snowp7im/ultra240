#include <dlfcn.h>
#include <ultra240/dynamic_library.h>
#include <stdexcept>
#include <string>

namespace ultra::posix {

  class DynamicLibrary : public ultra::DynamicLibrary {
    void* handle;
  public:
    DynamicLibrary(const char* name);
    ~DynamicLibrary();
    void close();
    void* load_symbol(const char* name);
  };

  DynamicLibrary::DynamicLibrary(const char* name)
    : handle(dlopen(name, RTLD_LAZY)) {
    if (handle == nullptr) {
      throw std::runtime_error(
        "DynamicLibrary: could not open library: " + std::string(dlerror())
      );
    }
  }

  DynamicLibrary::~DynamicLibrary() {
    if (handle != nullptr) {
      close();
    }
  }

  void DynamicLibrary::close() {
    dlclose(handle);
    handle = nullptr;
  }

  void* DynamicLibrary::load_symbol(const char* name) {
    return dlsym(handle, name);
  }

}

namespace ultra {

  void DynamicLibrary::init() {
  }

  void DynamicLibrary::quit() {
  }

  DynamicLibrary* DynamicLibrary::create(const char* name) {
    return new ultra::posix::DynamicLibrary(name);
  }

}
