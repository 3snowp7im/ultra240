#pragma once

#include <array>
#include <cstdint>
#include <list>
#include <memory>
#include <ultra240/geometry.h>
#include <ultra240/hash.h>
#include <ultra240/tileset.h>
#include <ultra240/world.h>
#include <utility>
#include <vector>

namespace ultra {

  /**
   * Base entity class.
   *
   * Entities are dynamic objects in the world.
   */
  class Entity {
  public:

    /**
     * Structure defining controls of entity animations.
     */ 
    struct AnimationControls {

      /** Signal defining how the animation cycle loops. */
      enum Cycle {
        /** Animation does not loop. */
        None,
        /** Animation loops. */
        Loop,
      } cycle;

      /** Signal defining how animation frames advance. */
      enum Direction {
        /** Animation tiles are incremented. */
        Forward,
        /** Animation tiles are decremented. */
        Backward,
      } direction;

      /** Instance compare operator. */
      bool operator==(const AnimationControls& rhs) const;

      /** Name of target animation. */
      Hash<>::Type name;

      /** Animation speed multiplier. */
      float speed;

      /** Instance constructor. */
      AnimationControls();

      /** Instance constructor with default controls. */
      AnimationControls(Hash<>::Type name);

      /** Instance builder. */
      class Builder {

        struct {
          Hash<>::Type name;
          Cycle cycle;
          Direction direction;
          float speed;
        } value;

      public:

        /** Builder instance constructor. */
        Builder();

        /** Set the name of the target animation. */
        Builder& name(Hash<>::Type name);

        /** Enable animation looping. */
        Builder& loop();

        /** Set tile incrementing. */
        Builder& forward();

        /** Set tile decrementing. */
        Builder& backward();

        /** Set animation speed. */
        Builder& speed(float speed);

        /** Return build instance. */
        AnimationControls build() const;
      };
    };

    /** Rendering attributes. */
    struct Attributes {
      /** Entity is rendered flipped about its x-axis. */
      bool flip_x = false;
      /** Entity is rendered flipped about its y-axis. */
      bool flip_y = false;
    };

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
      Hash<>::Type name;

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
      Attributes attributes,
      Args...
    );

    /** Generic factory to create instances from a specified tileset. */
    template <typename T = Entity, typename... Args>
    static T* from_tileset(
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes = {},
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
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes,
      Hash<>::Type type,
      uint16_t tile_index = 0
    );

    /** Animated instance constructor. */
    Entity(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes,
      Hash<>::Type type,
      AnimationControls animation_controls
    );

    /** Instance destructor. */
    virtual ~Entity();

    /** Return true if entity has collision boxes of a specified type. */
    virtual bool has_collision_boxes(Hash<>::Type type) const;

    /** Return collision boxes of a specified type. */
    virtual const Tileset::Tile::CollisionBox::NamedList& get_collision_boxes(
      Hash<>::Type type
    ) const;

    /** 
     * Return the position of an entity's collision box based on the entity's
     * position and rendering attributes.
     */
    virtual geometry::Vector<float> get_collision_box_position(
      const Tileset::Tile::CollisionBox& box
    ) const;

    /** 
     * Return the position of an entity's collision box based on a specified
     * position and the entity's rendering attributes.
     */
    virtual geometry::Vector<float> get_collision_box_position(
      const geometry::Vector<float>& pos,
      const Tileset::Tile::CollisionBox& box
    ) const;

    /**
     * Set entity's current animation. Will not restart if the target animation
     * is the current animation unless specified.
     */
    virtual bool animate(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries,
      AnimationControls animation_controls,
      bool force_restart = false
    );

    /** Update the current animation tile. */
    virtual bool update_animation(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries
    );

    /**
     * Find a collision between the entity and a boundary, if any. If there are
     * no collision, the first element of the returned pair is false, and the
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

    const Tileset& tileset;

    geometry::Vector<float> position;

    Attributes attributes;

    Hash<>::Type type;

    uint16_t tile_index;

    struct {
      bool playing;
      const Tileset::Tile* tile;
      AnimationControls animation_controls;
      uint32_t counter;
      std::vector<Tileset::Tile::AnimationTile>::const_iterator animation_tile;
    } animation;
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
        uint32_t
      >>("create_entity")(
        boundaries,
        *entity.tileset,
        entity.position,
        attributes,
        entity.tile_index,
        entity.state
      );
    }
    return entity.tileset->library->load_symbol<Entity::create<
      T,
      uint16_t,
      uint32_t
    >>("create_entity")(
      boundaries,
      *entity.tileset,
      entity.position,
      attributes,
      entity.tile_index,
      entity.state
    );
  }

}
