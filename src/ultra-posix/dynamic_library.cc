#include <dlfcn.h>
#include <ultra240/dynamic_library.h>
#include <stdexcept>
#include <string>

namespace ultra::posix {

  class DynamicLibraryImpl : public ultra::DynamicLibrary::Impl {
  public:

    static DynamicLibraryImpl*
    deref(std::unique_ptr<DynamicLibrary::Impl>& impl) {
      return reinterpret_cast<DynamicLibraryImpl*>(impl.get());
    }

    DynamicLibraryImpl(
      const PathManager& pm,
      const char* name
    ) : handle(dlopen(pm.get_lib_path(name).c_str(), RTLD_LAZY)) {
      if (handle == nullptr) {
        throw std::runtime_error(
          "DynamicLibrary: could not open library: " + std::string(dlerror())
        );
      }
    }

    ~DynamicLibraryImpl() {
      dlclose(handle);
    }

    void* load_symbol(const char* name) {
      return dlsym(handle, name);
    }

    void* handle;
  };

}

namespace ultra {

  using namespace ultra::posix;

  void DynamicLibrary::init() {}

  void DynamicLibrary::quit() {}

  DynamicLibrary::DynamicLibrary(const PathManager& pm, const char* name)
    : impl(new ultra::posix::DynamicLibraryImpl(pm, name)) {}

  DynamicLibrary::~DynamicLibrary() {
    close();
  }

  void DynamicLibrary::close() {
    impl = nullptr;
  }

  void* DynamicLibrary::load_symbol(const char* name) {
    auto impl = DynamicLibraryImpl::deref(this->impl);
    return impl->load_symbol(name);
  }

}
