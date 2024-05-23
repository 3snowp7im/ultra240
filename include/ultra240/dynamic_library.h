#pragma once

#include <memory>
#include <string>

namespace ultra {

  /** System independent dynamic library loader. */
  class DynamicLibrary {
  public:

    virtual ~DynamicLibrary() {};

    /** 
     * Load a symbol from library and return it cast as a pointer to the
     * specified type.
     */
    template <typename T>
    T* load_symbol(const std::string& name) {
      return reinterpret_cast<T*>(load_symbol(name));
    }

  private:

    virtual void* load_symbol(const std::string& name) = 0;
  };

}
