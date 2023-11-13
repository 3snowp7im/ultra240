#include <ultra240/config.h>
#include <ultra240/dynamic_library.h>
#include <ultra240/file.h>
#include <ultra240/path_manager.h>
#include <ultra240/settings_manager.h>
#include <ultra240/util.h>
#include <ultra240/window.h>

using namespace ultra;

using Main = int(Window&, PathManager&);

extern "C" int ultra_run(const char* name, int argc, const char* argv[]) {
  // Create resource loader and load the main config.
  file::init();
  config::init();
  PathManager::init();
  PathManager pm(name);
  config::Node config(pm.get_data_path("main.yaml").c_str());
  // Load the main module.
  DynamicLibrary::init();
  DynamicLibrary main(pm, config["main"].to_string().c_str());
  // Create a settings manager and load the user settings.
  SettingsManager::init();
  SettingsManager settings(pm, name);
  // Get window settings.
  SettingsManager window_settings(settings, "window");
  // Get controls settings.
  SettingsManager controls_settings(settings, "controls");
  // Create the window.
  auto title = config["title"].to_string();
  Window::init();
  Window window(title.c_str(), window_settings, controls_settings);
  // Run the main module.
  int ret = main.load_symbol<Main>("ultra_main")(window, pm);
  // Cleanup.
  window.close();
  Window::quit();
  main.close();
  DynamicLibrary::quit();
  SettingsManager::quit();
  PathManager::quit();
  config::quit();
  file::quit();
  return ret;
}
