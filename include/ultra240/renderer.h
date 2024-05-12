#pragma once

#include <vector>
#include <ultra240/entity.h>
#include <ultra240/geometry.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra::renderer {

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

  /** Unload the world from the graphics hardware. */
  void unload_world();

  /**
   * Width of the tileset texture.
   */
  inline constexpr uint16_t texture_width = 2048;

  /**
   * Height of the tileset texture.
   */
  inline constexpr uint16_t texture_height = 2048;

  /**
   * Number of layers in the tileset texture.
   */
  inline constexpr uint16_t texture_count = 64;

  /** Get the handle to the tilesets texture in hardware. */
  uintptr_t get_texture();

  /** 
   * Get the view transform for a map layer.
   *
   * This transform should be applied to all vertices for the specified layer.
   * The camera position is relative to the current map's top-left corner.
   */
  void get_view_transform(
    Transform view,
    const geometry::Vector<float>& camera_position,
    size_t layer_index
  );

  /** 
   * Get the projection transform matrix.
   *
   * This transform should be applied to all rendered vertices.
   */
  void get_projection_transform(
    Transform proj
  );

  /** Get the number of map tiles per layer. */
  size_t get_tile_count();

  /** 
   * Get matrices for map tile layer vertex and texture transforms.
   *
   * This function will write at most `transforms_count` matrices to each
   * output. Each transform returned should be applied to the 4 vertices of a
   * unit quad with vertices (0, 0, 1), (0, 1, 1), (1, 0, 1), and (1, 1, 1).
   */
  size_t get_map_transforms(
    Transform vertex_transforms[],
    Transform tex_transforms[],
    size_t transforms_count,
    size_t layer_index
  );

  /** Get the number of sprites contained by the specified handle. */
  size_t get_sprite_count(
    const std::vector<const SpriteHandle*>& sprites
  );

  /**
   * Get matrices for the sprite vertex and texture transforms.
   *
   * This function will write at most `transforms_count` matrices to each
   * output. Each transform returned should be applied to the 4 vertices of a
   * unit quad with vertices (0, 0, 1), (0, 1, 1), (1, 0, 1), and (1, 1, 1).
   */
  size_t get_sprite_transforms(
    Transform vertex_transforms[],
    Transform tex_transforms[],
    size_t transforms_count,
    const std::vector<const SpriteHandle*>& sprites,
    size_t layer_index
  );

  /** Advance frame counter. */
  void advance();

}
