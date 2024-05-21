#pragma once

#include <ultra240/geometry.h>
#include <ultra240/renderer.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>

namespace ultra {

  /**
   * Base entity class.
   *
   * Entities are dynamic objects in the world.
   */
  class Entity {
  public:

    /** Entity factory class. */
    template <typename T = Entity>
    class Factory {
    public:

      /** Generic instance constructor. */
      template <typename... Args>
      using create = T*(Args... args);

      /** Generic factory to create instances from a specified tileset. */
      template <typename... Args>
      static T* from_tileset(
        const Tileset& tileset,
        const renderer::TilesetHandle* handle,
        const geometry::Vector<float>& position,
        Tileset::Attributes attributes = {},
        Args... args
      );

      /** Generic factory to create instances from specified map data. */
      template <typename... Args>
      static T* from_map(
        const World::Map::Entity& entity,
        const renderer::TilesetHandle* handle,
        Args... args
      );

    };

    /** Instance constructor. */
    Entity();

    /** Instance destructor. */
    virtual ~Entity();
  };

  template <typename T>
  template <typename... Args>
  inline T* Entity::Factory<T>::from_tileset(
    const Tileset& tileset,
    const renderer::TilesetHandle* handle,
    const geometry::Vector<float>& position,
    Tileset::Attributes attributes,
    Args... args
  ) {
    return tileset.library->load_symbol<Entity::Factory<T>::create<
      const Tileset&,
      const renderer::TilesetHandle*,
      const geometry::Vector<float>&,
      Tileset::Attributes,
      Args...
    >>("create_entity")(
      tileset,
      handle,
      position,
      attributes,
      args...
    );
  }

  template <typename T>
  template <typename... Args>
  inline T* Entity::Factory<T>::from_map(
    const World::Map::Entity& entity,
    const renderer::TilesetHandle* handle,
    Args... args
  ) {
    Tileset::Attributes attributes = {
      .flip_x = entity.attributes.flip_x,
      .flip_y = entity.attributes.flip_y
    };
    const Tileset::Tile& tile = entity.tileset.tiles[entity.tile_index];
    if (tile.library != nullptr) {
      return tile.library->load_symbol<Entity::Factory<T>::create<
        const Tileset&,
        const renderer::TilesetHandle*,
        const geometry::Vector<float>&,
        Tileset::Attributes,
        uint16_t,
        uint16_t,
        uint16_t,
        uint32_t,
        Args...
      >>("create_entity")(
        entity.tileset,
        handle,
        entity.position,
        attributes,
        entity.tile_index,
        entity.type,
        entity.id,
        entity.state,
        args...
      );
    }
    return entity.tileset.library->load_symbol<Entity::Factory<T>::create<
      const Tileset&,
      const renderer::TilesetHandle*,
      const geometry::Vector<float>&,
      Tileset::Attributes,
      uint16_t,
      uint16_t,
      uint16_t,
      uint32_t,
      Args...
    >>("create_entity")(
      entity.tileset,
      handle,
      entity.position,
      attributes,
      entity.tile_index,
      entity.type,
      entity.id,
      entity.state,
      args...
    );
  }

}
