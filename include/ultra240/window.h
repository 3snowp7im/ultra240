#pragma once

#include <chrono>
#include <ultra240/entity.h>
#include <ultra240/event.h>
#include <ultra240/geometry.h>
#include <ultra240/resource_loader.h>
#include <ultra240/settings_manager.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra {

  class Window {

    std::chrono::time_point<std::chrono::steady_clock> last_render_time;

    std::chrono::time_point<std::chrono::steady_clock> last_sample_time;

    std::chrono::time_point<std::chrono::steady_clock> curr_time;

    int frames_rendered = 0;

  public:

    struct TilesetRange;

    struct EntityRange;

    struct BoxRange;

    static void init();

    static void quit();

    static Window* create(
      const char* title,
      SettingsManager& window_settings,
      SettingsManager& controls_settings
    );

    virtual ~Window() {}

    virtual void close() = 0;

    virtual bool should_quit() = 0;

    virtual bool poll_events(Event& event) = 0;

    virtual void set_camera_position(
      const geometry::Vector<float>& position
    ) = 0;

    virtual const TilesetRange* load_tilesets(
      ResourceLoader& loader,
      const std::vector<const Tileset*>& tilesets
    ) = 0;

    virtual void unload_tilesets(
      const std::vector<const TilesetRange*>& ranges
    ) = 0;

    virtual void load_world(ResourceLoader& loader, const World& world) = 0;

    virtual void unload_world() = 0;

    virtual const TilesetRange* load_map(uint16_t index) = 0;

    virtual const EntityRange* load_entities(
      const std::vector<const Entity*>& entities,
      const std::vector<const TilesetRange*>& tilesets
    ) = 0;

    virtual void unload_entities(
      const std::vector<const EntityRange*>& ranges
    ) = 0;

    virtual const BoxRange* load_boxes(
      const std::vector<geometry::Rectangle<float>>& boxes
    ) = 0;

    virtual void unload_boxes(
      const std::vector<const BoxRange*> ranges
    ) = 0;

    virtual void draw() = 0;

  protected:

    void predraw();

    void postdraw();
  };

}
