#pragma once

#include <memory>
#include <ultra240/file.h>

namespace ultra {

  /** System independent filesystem path manager. */
  class PathManager {
  public:

    /** Perform module initialization. Must be called before instantiation. */
    static void init();

    /** Free module resources. */
    static void quit();

    /** Create an instance loading resources for the specified name. */
    PathManager(const char* name);

    /** Free instance resources. */
    ~PathManager();

    /** Return the path of the specified user file component. */
    std::string get_user_path(const char* name = nullptr) const;

    /** Return the path of a specified data file component. */
    std::string get_data_path(const char* name = nullptr) const;

    /** Return the path of a specified code library. */
    std::string get_lib_path(const char* name = nullptr) const;

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    std::unique_ptr<Impl> impl;
  };

}
