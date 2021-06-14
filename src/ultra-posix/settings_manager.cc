#include <iostream>
#include <memory>
#include <ultra240/file.h>
#include <ultra240/settings_manager.h>
#include "mkdirp.h"

namespace ultra::posix {

  static const char* settings_filename = "settings.yaml";

  class SettingsManager : public ultra::SettingsManager {
    const std::string config_path;
    const std::string settings_path;
    YAML::Node settings;
    ultra::SettingsManager* const root;
    SettingsManager(YAML::Node settings, ultra::SettingsManager* root);
  public:
    SettingsManager(const char* name);
    ultra::SettingsManager* manage_map(const char* key);
    YAML::Node operator[](const char* key);
    void remove(const char* key);
    void save();
  };

  SettingsManager::SettingsManager(
    YAML::Node settings,
    ultra::SettingsManager* root
  ) : settings(settings), root(root) {}

  static YAML::Node get_settings(
    const std::string& config_path,
    const std::string& settings_path
  ) {
    mkdirp(config_path.c_str());
    try {
      std::shared_ptr<file::Input> file(
        file::Input::open(settings_path.c_str())
      );
      return YAML::Load(file->istream());
    } catch (std::exception& e) {
      std::cerr << "SettingsManager: error loading user settings" << std::endl;
    }
    return YAML::Node(YAML::NodeType::value::Map);
  }

  static std::string get_config_path(const char* name) {
    return std::string(getenv("HOME")) + "/.config/" + name;
  }

  SettingsManager::SettingsManager(const char* name)
    : settings_path(get_config_path(name) + "/" + settings_filename),
      settings(get_settings(get_config_path(name), settings_path)),
      root(nullptr) {
  }

  ultra::SettingsManager* SettingsManager::manage_map(const char* key) {
    auto map = settings[key];
    if (!map.IsDefined()) {
      settings[key] = YAML::Node(YAML::NodeType::value::Map);
      map = settings[key];
    }
    return new SettingsManager(map, root == nullptr ? this : root);
  }

  YAML::Node SettingsManager::operator[](const char* key) {
    return settings[key];
  }

  void SettingsManager::remove(const char* key) {
    settings.remove(key);
  }

  void SettingsManager::save() {
    if (root == nullptr) {
      YAML::Emitter out;
      out << settings;
      std::shared_ptr<file::Output> file(
        file::Output::open(settings_path.c_str())
      );
      file->ostream() << out.c_str();
      file->close();
    } else {
      root->save();
    }
  }

}

namespace ultra {

  void SettingsManager::init() {}

  void SettingsManager::quit() {}

  SettingsManager* SettingsManager::create(const char* name) {
    return new posix::SettingsManager(name);
  }

}
