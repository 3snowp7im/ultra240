#pragma once

#include <memory>
#include <ultra240/dynamic_library.h>

namespace ultra::dynamic_library {

  void init();
  
  void quit();

  class Impl : public DynamicLibrary {
  public:

    static Impl* deref(std::unique_ptr<DynamicLibrary>& library) {
      return reinterpret_cast<Impl*>(library.get());
    }

    Impl(const std::string& name);

    ~Impl();

    void close();

    void* load_symbol(const std::string& name);

    class SystemImpl {
    public:
      virtual ~SystemImpl() {};
    };

    std::unique_ptr<SystemImpl> impl;
  };

}
