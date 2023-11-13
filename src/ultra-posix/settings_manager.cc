#include <memory>
#include <ultra240/config.h>
#include <ultra240/file.h>
#include <ultra240/settings_manager.h>
#include "mkdirp.h"

namespace ultra::posix {

  using namespace ultra;

  static const char* settings_filename = "settings.yaml";

  class SettingsManagerImpl : public SettingsManager::Impl {
  public:

    static SettingsManagerImpl* deref(
      std::unique_ptr<SettingsManager::Impl>& impl
    ) {
      return reinterpret_cast<SettingsManagerImpl*>(impl.get());
    }

    static config::Node get_settings(
      const std::string& config_path,
      const std::string& settings_path
    ) {
      mkdirp(config_path.c_str());
      try {
        return config::Node(settings_path.c_str());
      } catch (std::exception& e) {
      }
      return config::Node();
    }

    SettingsManagerImpl(config::Node node, SettingsManager* root)
      : node(node),
        root(root) {}

    SettingsManagerImpl(
      const PathManager& pm,
      const char* name,
      SettingsManager* root
    ) : settings_path(pm.get_user_path(settings_filename)),
        node(get_settings(pm.get_user_path(), settings_path)),
        root(root) {}

    config::Node node;

    SettingsManager* root;

    std::string settings_path;
  };

}

namespace ultra {

  using namespace ultra::posix;

  void SettingsManager::init() {}

  void SettingsManager::quit() {}

  SettingsManager::SettingsManager(const PathManager& pm, const char* name)
    : impl(new SettingsManagerImpl(pm, name, this)) {}

  SettingsManager::SettingsManager(SettingsManager& settings, const char* key)
    : impl(
      new SettingsManagerImpl(
        settings[key],
        SettingsManagerImpl::deref(settings.impl)->root
      )
    ) {
    auto impl = SettingsManagerImpl::deref(this->impl);
    if (!impl->node.is_defined()) {
      impl->node = config::Node();
      settings[key] = impl->node;
    }
  }

  SettingsManager::~SettingsManager() {}

  config::Node SettingsManager::operator[](const char* key) {
    auto impl = SettingsManagerImpl::deref(this->impl);
    return impl->node[key];
  }

  void SettingsManager::remove(const char* key) {
    auto impl = SettingsManagerImpl::deref(this->impl);
    impl->node.remove(key);
  }

  void SettingsManager::save() {
    auto impl = SettingsManagerImpl::deref(this->impl);
    if (impl->root == this) {
      file::Output file(impl->settings_path.c_str());
      impl->node.save(file.stream());
      file.close();
    } else {
      impl->root->save();
    }
  }

}
