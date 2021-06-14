#pragma once

#include <ultra240/file.h>

namespace ultra {

  class ResourceLoader {
  public:

    static void init();

    static void quit();

    static ResourceLoader* create(const char* name);

    virtual ~ResourceLoader() {}

    virtual std::string get_data_path(const char* path) = 0;

    virtual std::string get_lib_path(const char* path) = 0;

    virtual file::Input* open_data(const char* path) = 0;

    virtual file::Input* open_lib(const char* path) = 0;
  };

}
