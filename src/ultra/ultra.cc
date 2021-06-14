#include <memory>
#include <ultra240/dynamic_library.h>
#include <ultra240/file.h>
#include <ultra240/resource_loader.h>
#include <ultra240/settings_manager.h>
#include <ultra240/util.h>
#include <ultra240/window.h>
#include <yaml-cpp/yaml.h>

using namespace ultra;

using Main = int(Window&, ResourceLoader&);

extern "C" int ultra_run(const char* name, int argc, const char* argv[]) {
  // Create resource loader and load the main config.
  file::init();
  ResourceLoader::init();
  std::shared_ptr<ResourceLoader> resources(ResourceLoader::create(name));
  std::shared_ptr<file::Input> main_config(
    resources->open_data("main.yaml")
  );
  auto config = YAML::Load(main_config->istream());
  main_config = nullptr;
  // Load the main module.
  DynamicLibrary::init();
  std::shared_ptr<DynamicLibrary> main(
    util::load_library(*resources, config["main"].as<std::string>().c_str())
  );
  // Create a settings manager and load the user settings.
  SettingsManager::init();
  std::shared_ptr<SettingsManager> settings(SettingsManager::create(name));
  // Get window settings.
  std::shared_ptr<SettingsManager> window_settings(
    settings->manage_map("window")
  );
  // Get controls settings.
  std::shared_ptr<SettingsManager> controls_settings(
    settings->manage_map("controls")
  );
  // Create the window.
  auto title = config["title"].as<std::string>();
  Window::init();
  std::shared_ptr<Window> window(
    Window::create(title.c_str(), *window_settings, *controls_settings)
  );
  // Run the main module.
  int ret = main->load_symbol<Main>("ultra_main")(
    *window,
    *resources
  );
  // Cleanup.
  window = nullptr;
  Window::quit();
  main = nullptr;
  DynamicLibrary::quit();
  settings = nullptr;
  SettingsManager::quit();
  resources = nullptr;
  ResourceLoader::quit();
  file::quit();
  return ret;
}
