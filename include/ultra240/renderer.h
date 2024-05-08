#pragma once

#include <vector>
#include <ultra240/entity.h>
#include <ultra240/geometry.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra::renderer {

  /** Get the handle to the texture the renderer draws into. */
  uintptr_t get_render_texture();

  /** Internal pointer to tileset loaded in graphics hardware. */
  struct TilesetHandle;

  /** Internal pointer to sprites loaded in graphics hardware. */
  struct SpriteHandle;

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
  const SpriteHandle* load_entities(
    const std::vector<const Entity*>& entities,
    const std::vector<const TilesetHandle*>& tilesets
  );

  /** Unload a collection of entities from the graphics hardware. */
  void unload_entities(const std::vector<const SpriteHandle*>& handles);

  /** Load a world map for rendering. */
  const TilesetHandle* load_map(uint16_t index);

  /** Load a world to the graphics hardware. */
  void load_world(const World& world);

  /** Unload the world from the hraphics hardware. */
  void unload_world();

  /** Clear the render texture. */
  void clear();

  /** Render tile layers. */
  void render_tile_layers(
    size_t start_layer_idx,
    ssize_t layer_count
  );

  /** Render sprites. */
  void render_sprites(
    const std::vector<const SpriteHandle*>& sprites,
    size_t layer_index
  );

  /** Advance frame counter. */
  void advance();

}
