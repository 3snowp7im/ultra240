#pragma once

#include <memory>
#include <ultra240/path_manager.h>

namespace ultra {

  /**
   * System independent dynamic library loader.
   */
  class DynamicLibrary {
  public:

    /** Perform module initialization. Must be called before instantiation. */
    static void init();

    /** Free module resources. */
    static void quit();

    /** Load the library specified by name. */
    DynamicLibrary(const PathManager& pm, const char* name);

    /** Free instance resources. Automatically closes the library if opened. */
    ~DynamicLibrary();

    /**
     * Close the library. Attempting to load symbols from a closed library
     * results in undefined behaviour.
     */
    void close();

    /** Return a pointer of type T to the library symbol specified by name. */
    template <typename T>
    T* load_symbol(const char* name) {
      return reinterpret_cast<T*>(load_symbol(name));
    }

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    void* load_symbol(const char* name);

    std::unique_ptr<Impl> impl;
  };

}
