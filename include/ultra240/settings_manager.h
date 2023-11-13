#pragma once

#include <memory>
#include <ultra240/config.h>
#include <ultra240/path_manager.h>

namespace ultra {

  class SettingsManager {
  public:

    static void init();

    static void quit();

    SettingsManager(const PathManager& pm, const char* name);

    SettingsManager(SettingsManager& settings, const char* key);

    ~SettingsManager();

    config::Node operator[](const char* key);

    void remove(const char* key);

    void save();

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    std::unique_ptr<Impl> impl;
  };

}
