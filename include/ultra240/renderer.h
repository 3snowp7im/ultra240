#pragma once

#include <vector>
#include <ultra240/entity.h>
#include <ultra240/geometry.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra::renderer {

  inline constexpr uint16_t texture_width = 2048;
  inline constexpr uint16_t texture_height = 2048;
  inline constexpr uint16_t texture_count = 64;

  /** A 4x4 transformation matrix in column major order. */
  using Transform = float[16];

  /** Internal pointer to tileset loaded in graphics hardware. */
  struct TilesetHandle;

  /** Internal pointer to sprites loaded in graphics hardware. */
  struct SpriteHandle;

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

  /** Set the current world map for rendering. */
  const TilesetHandle* set_map(uint16_t index);

  /** Load a world to the graphics hardware. */
  void load_world(const World& world);

  /** Unload the world from the hraphics hardware. */
  void unload_world();

  /** Get the handle to the tilesets texture in hardware. */
  uintptr_t get_texture();

  /** Get the projection transform matrix. */
  void get_projection_transform(
    Transform proj
  );

  /** Get the number of map tiles. */
  size_t get_tile_count(
    size_t start_layer_idx,
    ssize_t layer_count = -1
  );

  /** Get matrices for map tile layers quad transforms. */
  size_t get_tile_transforms(
    Transform quad_transforms[],
    Transform tex_transforms[],
    size_t transforms_count,
    const geometry::Vector<float>& camera_position,
    size_t start_layer_idx,
    ssize_t layer_count = -1
  );

  /** Get the number of sprites. */
  size_t get_sprite_count(
    const std::vector<const SpriteHandle*>& sprites
  );

  /** Get matrices for the sprite quad transforms. */
  size_t get_sprite_transforms(
    Transform quad_transforms[],
    Transform tex_transforms[],
    size_t transforms_count,
    const geometry::Vector<float>& camera_position,
    const std::vector<const SpriteHandle*>& sprites,
    size_t layer_index
  );

  /** Advance frame counter. */
  void advance();

}
