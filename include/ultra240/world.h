#pragma once

#include <istream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include <ultra240/hash.h>
#include <ultra240/geometry.h>
#include <ultra240/tileset.h>
#include <ultra240/vector_allocator.h>

namespace ultra {

  /** 
   * World class.
   *
   * A world is a collection of interconnected maps.
   */
  class World {
  public:

    /** 
     * Map class.
     *
     * A map is a collection of layers, entities, and their respective tilesets.
     */
    class Map {
    public:

      /** Map layer class. */
      class Layer {
      public:

        /** Read a serialized layer from a stream. */
        Layer(std::istream& stream);

        /** Name of this layer. */
        ultra::Hash name;

        /** The rendered parallax of the layer. */
        geometry::Vector<float> parallax;
      };

      /** 
       * Map entity class.
       *
       * A map entity specifies an entity tileset and its initial state.
       */
      class Entity {
      public:

        /** Instance constructor */
        Entity(
          const std::shared_ptr<Tileset>* tileset,
          std::istream& stream
        );

        /** Name of the layer entity is on. */
        Hash layer_name;

        /** The initial entity position in pixels. */
        geometry::Vector<uint16_t> position;

        /** The initial tile index. */
        uint16_t tile_index;

        /** Entity type. */
        uint16_t type;

        /** Entity ID. */
        uint16_t id;

        /** 
         * The initial entity state data.
         *
         * There is no standard format for the entity state, it is left up to
         * the application to interpret it.
         */
        uint32_t state;

        /** Pointer to the entity tileset. */
        const Tileset& tileset;

        /** The initial entity attributes. */
        struct {
          bool flip_x;
          bool flip_y;
        } attributes;
      };

      /** Read a serialized map from a stream. */
      Map(std::istream& stream);

      /** Position of the map in the world in tile units. */
      geometry::Vector<int16_t> position;

      /** Map dimensions in tile units. */
      geometry::Vector<uint16_t> size;

      /** Map properties. */
      std::unordered_map<Hash, uint32_t> properties;

      /** Collection of map tilesets. */
      std::vector<std::shared_ptr<Tileset>> map_tilesets;

      /** Collection of entity tilesets. */
      std::vector<std::shared_ptr<Tileset>> entity_tilesets;

      /** Collection of map layers. */
      std::vector<Layer> layers;

      /** Collection of map entities. */
      std::vector<Entity> entities;

      /** Collections of entity indices sorted by position. */
      struct {
        struct {
          std::vector<uint16_t> min, max;
        } x, y;
      } sorted_entities;

      /** The tile IDs comprising the map. */
      std::vector<uint16_t> tiles;
    };

    /** 
     * Boundary class.
     *
     * A boundary is a line segment defining the map boundaries where entity
     * collision is enforced.
     */
    class Boundary : public geometry::LineSegment<float> {
    public:

      /** Boundary flags. */
      enum Flags {

        /**
         * One-way boundary flag.
         *
         * Collision is only checked when the dot product of the boundary normal
         * and the entity movement vector is positive. In this manner, all
         * boundaries are one-way, however, this flag disables transient
         * animation tile collision checking when the entity is currently
         * intersecting the boundary. This prevents unwanted zipping during
         * entity animations.
         */
        OneWay = 0x40,
      };

      /** Instance constructor. */
      Boundary();

      /** Construct boundary from two vectors. */
      Boundary(
        const geometry::Vector<float>& p,
        const geometry::Vector<float>& q
      );

      /** Construct boundary from two vectors and flags. */
      Boundary(
        uint8_t flags,
        const geometry::Vector<float>& p,
        const geometry::Vector<float>& q
      );

      /** Boundary flags. */
      uint8_t flags;
    };

    /** General structure describing a collision. */
    struct Collision {

      /** Signal defining which collision box edge collision occurred on. */
      enum Edge {
        Top,
        Right,
        Bottom,
        Left,
      } edge;

      /** Name of the collision box on which collision occurred on. */
      Hash name;

      /** Distance from collision. */
      geometry::Vector<float> distance;
    };

    /** Fixed size vector backed list of boundaries. */
    using Boundaries = VectorAllocatorList<Boundary>;

    /** Structure describing an entity collision with a boundary. */
    struct BoundaryCollision : Collision {

      /** Iterator pointing to the boundary the entity collided with. */
      typename Boundaries::const_iterator boundary;
    };

    /**
     * Find a collision between collision boxes and a boundary, if any. If
     * there are no collisions, the first element of the returned pair is false,
     * and the collision data contains no information.
     */
    static std::pair<bool, BoundaryCollision> get_boundary_collision(
      geometry::Vector<float> force,
      const Tileset::Tile::CollisionBox<float>* collision_boxes,
      size_t collision_boxes_count,
      const Boundaries& boundaries
    );

    /**
     * Determines if a new collection of collision boxes can fit in within the
     * specified boundries. If so, the first element of the returned pair is
     * true and the second is the position offset required to make the fit.
     */
    static std::pair<bool, geometry::Vector<float>> can_fit_collision_boxes(
      const Tileset::Tile::CollisionBox<float>* prev_collision_boxes,
      size_t prev_collision_boxes_count,
      const Tileset::Tile::CollisionBox<float>* next_collision_boxes,
      size_t next_collision_boxes_count,
      const Boundaries& boundaries,
      bool check_transits
    );

    /** Load serialized world from specified file name. */
    World(const std::string& name);

    /** Get the boundaries collection. */
    const Boundaries& get_boundaries() const;

    /** Collection of world maps. */
    std::vector<Map> maps;

  private:

    std::unique_ptr<Boundaries> boundaries;
  };

}
