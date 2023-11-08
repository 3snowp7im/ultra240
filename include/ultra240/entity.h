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

  class Entity {
  public:

    struct AnimationControls {

      enum Cycle {
        None,
        Loop,
      } cycle;

      enum Direction {
        Forward,
        Backward,
      } direction;

      bool operator==(const AnimationControls& rhs) const;

      Hash<>::Type name;

      float speed;

      AnimationControls();

      AnimationControls(Hash<>::Type name);

      class Builder {

        struct {
          Hash<>::Type name;
          Cycle cycle;
          Direction direction;
          float speed;
        } value;

      public:

        Builder();

        Builder& name(Hash<>::Type name);

        Builder& loop();

        Builder& forward();

        Builder& backward();

        Builder& speed(float speed);

        AnimationControls build() const;
      };
    };

    struct Attributes {
      bool flip_x = false;
      bool flip_y = false;
    };

    struct Collision {

      enum Edge {
        Top,
        Right,
        Bottom,
        Left,
      } edge;

      Hash<>::Type name;

      geometry::Vector<float> distance;

      typename World::Boundaries::const_iterator boundary;
    };

    template <typename T = Entity, typename... Args>
    using create = T*(
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes,
      Args...
    );

    template <typename T = Entity, typename... Args>
    static T* from_tileset(
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes = {},
      Args... args
    );

    template <typename T = Entity>
    static T* from_map(
      const World::Boundaries& boundaries,
      const World::Map::Entity& entity
    );

    Entity(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes,
      Hash<>::Type type,
      uint16_t tile_index = 0
    );

    Entity(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries,
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes,
      Hash<>::Type type,
      AnimationControls animation_controls
    );

    virtual ~Entity();

    virtual bool has_collision_boxes(Hash<>::Type type) const;

    virtual const Tileset::Tile::CollisionBox::NamedList& get_collision_boxes(
      Hash<>::Type type
    ) const;

    virtual geometry::Vector<float> get_collision_box_position(
      const Tileset::Tile::CollisionBox& box
    ) const;

    virtual geometry::Vector<float> get_collision_box_position(
      const geometry::Vector<float>& pos,
      const Tileset::Tile::CollisionBox& box
    ) const;

    virtual bool animate(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries,
      AnimationControls animation_controls,
      bool force_restart = false
    );

    virtual bool update_animation(
      Hash<>::Type collision_box_type,
      const World::Boundaries& boundaries
    );

    virtual std::pair<bool, Collision> get_collision(
      geometry::Vector<float> force,
      const Tileset::Tile::CollisionBox::NamedList& collision_boxes,
      const World::Boundaries& boundaries
    );

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
