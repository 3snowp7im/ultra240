#pragma once

namespace ultra {

  class DynamicLibrary {

    virtual void* load_symbol(const char* name) = 0;

  public:

    static void init();

    static void quit();

    static DynamicLibrary* create(const char* name);

    virtual ~DynamicLibrary() {}

    virtual void close() = 0;

    template <typename T>
    T* load_symbol(const char* name) {
      return reinterpret_cast<T*>(load_symbol(name));
    }
  };

}
