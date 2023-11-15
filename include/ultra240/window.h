#pragma once

#include <chrono>
#include <ultra240/entity.h>
#include <ultra240/event.h>
#include <ultra240/geometry.h>
#include <ultra240/path_manager.h>
#include <ultra240/settings_manager.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra {

  /** The rendering window. */
  class Window {

    /** Timestamp of last render. */
    std::chrono::time_point<std::chrono::steady_clock> last_render_time;

    /** Timestamp of last sample. */
    std::chrono::time_point<std::chrono::steady_clock> last_sample_time;

    /** Current timestamp. */
    std::chrono::time_point<std::chrono::steady_clock> curr_time;

    /** Number of frames rendered. */
    int frames_rendered = 0;

  public:

    /** Perform module initialization. Must be called before instantiation. */
    static void init();

    /** Free module resources. */
    static void quit();

    /** Internal pointer to tilesets loaded in graphics hardware. */
    struct TilesetRange;

    /** Internal pointer to entities loaded in graphics hardware. */
    struct EntityRange;

    /** Internal pointer to boxes loaded in graphics hardware. */
    struct BoxRange;

    /** Instance constructor. */
    Window(
      const char* title,
      SettingsManager& window_settings,
      SettingsManager& controls_settings
    );

    /** Instance destructor. */
    ~Window();

    /** Close window. */
    void close();

    /** True if user has triggered a window close through system UI. */
    bool should_quit();

    /** Poll for new input events. */
    bool poll_events(Event& event);

    /** Set the rendering camera position. */
    void set_camera_position(
      const geometry::Vector<float>& position
    );

    /** Load a collection of tilesets to the graphics hardware. */
    const TilesetRange* load_tilesets(
      PathManager& loader,
      const std::vector<const Tileset*>& tilesets
    );

    /** Unload a collection of tilesets from the graphics hardware. */
    void unload_tilesets(
      const std::vector<const TilesetRange*>& ranges
    );

    /** Load a world to the graphics hardware. */
    void load_world(PathManager& pm, const World& world);

    /** Unload the world from the hraphics hardware. */
    void unload_world();

    /** Load a world map for rendering. */
    const TilesetRange* load_map(uint16_t index);

    /** 
     * Load a collection of entities and their associated tileset pointers
     * to the graphics hardware.
     */
    const EntityRange* load_entities(
      const std::vector<const Entity*>& entities,
      const std::vector<const TilesetRange*>& tilesets
    );

    /** Unload a collection of entities from the graphics hardware. */
    void unload_entities(const std::vector<const EntityRange*>& ranges);

    /** Load a collection of boxes to the graphics hardware. */
    const BoxRange* load_boxes(
      const std::vector<geometry::Rectangle<float>>& boxes
    );

    /** Unload a collection of boxes from the graphics hardware. */
    void unload_boxes(
      const std::vector<const BoxRange*> ranges
    );

    /** 
     * Bind the drawing context.
     *
     * This should only be called if the user wants to use external programs
     * for drawing. The window will automatically bind its drawing context
     * when necessary.
     */
    void bind_context();

    /** Clear the rendering context with the specified color. */
    void clear_context(float r, float g, float b, float a);

    /** Render the current frame to the context. */
    void render();

    /** Draw the context to the screen. */
    void draw_context();

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    std::unique_ptr<Impl> impl;

  protected:

    void predraw();

    void postdraw();
  };

}
