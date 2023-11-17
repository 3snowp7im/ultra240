#pragma once

#include <memory>
#include <string>

namespace ultra {

  /** System independent dynamic library loader. */
  class DynamicLibrary {
  public:

    virtual ~DynamicLibrary() {};

    template <typename T>
    T* load_symbol(const std::string& name) {
      return reinterpret_cast<T*>(load_symbol(name.c_str()));
    }

  private:

    virtual void* load_symbol(const std::string& name) = 0;
  };

}
