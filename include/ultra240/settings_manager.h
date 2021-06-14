#pragma once

#include <ultra240/file.h>
#include <yaml-cpp/yaml.h>

namespace ultra {

  class SettingsManager {
  public:

    static void init();

    static void quit();

    static SettingsManager* create(const char* name);

    virtual ~SettingsManager() {}

    virtual SettingsManager* manage_map(const char* key) = 0;

    virtual YAML::Node operator[](const char* key) = 0;

    virtual void remove(const char* key) = 0;

    virtual void save() = 0;
  };

}
