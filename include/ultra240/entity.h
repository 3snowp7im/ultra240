#pragma once

#include <list>
#include <memory>
#include <vector>
#include <ultra240/geometry.h>
#include <ultra240/hash.h>
#include <ultra240/sprite.h>
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

    /** General structure describing an entity collision. */
    struct Collision {

      /** Signal defining which collision box edge collision occurred on. */
      enum Edge {
        Top,
        Right,
        Bottom,
        Left,
      } edge;

      /** Name of the collision box collision occurred on. */
      Hash name;

      /** Distance from collision. */
      geometry::Vector<float> distance;
    };

    /** Structure describing an entity collision with a boundary. */
    struct BoundaryCollision : Collision {

      /** Iterator pointing to the boundary the entity collided with. */
      typename World::Boundaries::const_iterator boundary;
    };

    /** Generic instance constructor. */
    template <typename T = Entity, typename... Args>
    using create = T*(
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Sprite::Attributes attributes,
      Args...
    );

    /** Generic factory to create instances from a specified tileset. */
    template <typename T = Entity, typename... Args>
    static T* from_tileset(
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Sprite::Attributes attributes = {},
      Args... args
    );

    /** Generic factory to create instances from specified map data. */
    template <typename T = Entity>
    static T* from_map(
      const World::Boundaries& boundaries,
      const World::Map::Entity& entity
    );

    /** Static tile instance constructor. */
    Entity(
      Hash collision_box_type,
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Sprite::Attributes attributes,
      uint16_t tile_index = 0
    );

    /** Instance destructor. */
    virtual ~Entity();

    /** Return true if entity has collision boxes of a specified type. */
    virtual bool has_collision_boxes(Hash type) const = 0;

    /** Return collision boxes of a specified type. */
    virtual const Tileset::Tile::CollisionBox::NamedList& get_collision_boxes(
      Hash type
    ) const = 0;

    /** 
     * Return the position of an entity's collision box based on the entity's
     * position and attributes.
     */
    virtual geometry::Vector<float> get_collision_box_position(
      const Tileset::Tile::CollisionBox& box
    ) const;

    /** 
     * Return the position of an entity's collision box based on a specified
     * position and the entity's attributes.
     */
    virtual geometry::Vector<float> get_collision_box_position(
      const geometry::Vector<float>& pos,
      const Tileset::Tile::CollisionBox& box
    ) const;

    /**
     * Find a collision between the entity and a boundary, if any. If there are
     * no collisions, the first element of the returned pair is false, and the
     * the collision data contains no information.
     */
    virtual std::pair<bool, BoundaryCollision> get_boundary_collision(
      geometry::Vector<float> force,
      const Tileset::Tile::CollisionBox::NamedList& collision_boxes,
      const World::Boundaries& boundaries
    );

    /** Update entity state. */
    virtual void update(
      World::Boundaries& boundaries,
      Entity* player,
      const std::vector<Entity*>& entities
    );

    /** Get sprite count. */
    virtual size_t get_sprite_count();

    /** Get sprites. */
    virtual void get_sprites(Sprite* sprites);

    /** The entity world position. */
    geometry::Vector<float> position;
  };

  template <typename T = Entity, typename... Args>
  inline T* Entity::from_tileset(
    const World::Boundaries& boundaries,
    const Tileset& tileset,
    const geometry::Vector<float>& position,
    Attributes attributes,
    Args... args
  ) {
    return tileset.library->load_symbol<Entity::create<
      T,
      Args...
    >>("create_entity")(
      boundaries,
      tileset,
      position,
      attributes,
      args...
    );
  }

  template <typename T = Entity>
  inline T* Entity::from_map(
    const World::Boundaries& boundaries,
    const World::Map::Entity& entity
  ) {
    Attributes attributes = {
      .flip_x = entity.attributes.flip_x,
      .flip_y = entity.attributes.flip_y
    };
    const Tileset::Tile& tile = entity.tileset->tiles[entity.tile_index];
    if (tile.library != nullptr) {
      return tile.library->load_symbol<Entity::create<
        T,
        uint16_t,
        uint16_t,
        uint16_t,
        uint32_t
      >>("create_entity")(
        boundaries,
        *entity.tileset,
        entity.position,
        attributes,
        entity.tile_index,
        entity.type,
        entity.id,
        entity.state
      );
    }
    return entity.tileset->library->load_symbol<Entity::create<
      T,
      uint16_t,
      uint16_t,
      uint16_t,
      uint32_t
    >>("create_entity")(
      boundaries,
      *entity.tileset,
      entity.position,
      attributes,
      entity.tile_index,
      entity.type,
      entity.id,
      entity.state
    );
  }

}
