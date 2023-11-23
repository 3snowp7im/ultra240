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

      /**
       * Map layer class.
       *
       * A map layer is a collection of tiles.
       */
      class Layer {
      public:

        /** Read a serialized layer from a stream. */
        Layer(
          std::istream& stream,
          const geometry::Vector<uint16_t>& size
        );

        /** The rendered parallax of the layer. */
        geometry::Vector<float> parallax;

        /** The tile IDs comprising the layer. */
        std::vector<uint16_t> tiles;
      };

      /** 
       * Map entity class.
       *
       * A map entity specifies the entity tileset and its initial state.
       */
      class Entity {
      public:

        /** Read a serialized entity from a stream. */
        Entity(std::istream& stream);

        /** Pointer to the entity tileset. */
        const Tileset* tileset;

        /** The initial tile index. */
        uint16_t tile_index;

        /** The initial entity attributes. */
        struct {
          bool flip_x;
          bool flip_y;
        } attributes;

        /** The initial entity position. */
        geometry::Vector<uint32_t> position;

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
      };

      /** Read a serialized map from a stream. */
      Map(std::istream& stream);

      /** Position of the map in the world. */
      geometry::Vector<int16_t> position;

      /** Map dimensions. */
      geometry::Vector<uint16_t> size;

      /** Map properties. */
      std::unordered_map<Hash<>::Type, uint32_t> properties;

      /** The index of the entities layer. */
      uint8_t entities_index;

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
         * intersecting the boundary. This prevents zipping unwanted zipping
         * during entity animations.
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

    /** Fixed size vector backed list of boundaries. */
    using Boundaries = VectorAllocatorList<Boundary>;

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
