#pragma once

#include <vector>
#include <ultra240/entity.h>
#include <ultra240/geometry.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra::renderer {

  /** Set the frame buffer to render to. */
  void set_frame_buffer(void* frame_buffer);

  /** Internal pointer to tileset loaded in graphics hardware. */
  struct TilesetHandle;

  /** Internal pointer to entities loaded in graphics hardware. */
  struct EntityHandle;

  /** Set the rendering camera position. */
  void set_camera_position(const geometry::Vector<float>& position);

  /** Load a collection of tilesets to the graphics hardware. */
  const TilesetHandle* load_tilesets(
    const std::vector<const Tileset*>& tilesets
  );

  /** Unload a collection of tilesets from the graphics hardware. */
  void unload_tilesets(const std::vector<const TilesetHandle*>& handles);

  /** 
   * Load a collection of entities and their associated tileset handles
   * to the graphics hardware.
   */
  const EntityHandle* load_entities(
    const std::vector<const Entity*>& entities,
    const std::vector<const TilesetHandle*>& tilesets
  );

  /** Unload a collection of entities from the graphics hardware. */
  void unload_entities(const std::vector<const EntityHandle*>& handles);

  /** Load a world map for rendering. */
  const TilesetHandle* load_map(uint16_t index);

  /** Load a world to the graphics hardware. */
  void load_world(const World& world);

  /** Unload the world from the hraphics hardware. */
  void unload_world();

  /** Render the current frame to the frame buffer. */
  void render();

}
