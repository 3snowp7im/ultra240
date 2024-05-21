#pragma once

#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <ultra240/dynamic_library.h>
#include <ultra240/hash.h>
#include <ultra240/geometry.h>
#include <ultra240/vector_allocator.h>

namespace ultra {

  /**
   * Tileset class.
   *
   * A tileset contains the name of the bitmap used for rendering, individual
   * tile data, and an associated code library.
   */
  class Tileset {
  public:

    /** Render attributes. */
    struct Attributes {

      /** Flipped about its x-axis. */
      bool flip_x = false;

      /** Flipped about its y-axis. */
      bool flip_y = false;
    };

    /** 
     * Tile data class.
     *
     * Tile data can include collision boxes, animations, and an associtade code
     * library. A code library is useful on a per tile basis when a map tileset
     * is used to define entities.
     */
    class Tile {
    public:

      /** Collision box class. */
      template <typename T>
      class CollisionBox : public geometry::Rectangle<T> {
      public:

        /** Read serialized collision box from stream. */
        CollisionBox(
          Hash name,
          std::istream& stream
        );

        /** Create collision box from position and size. */
        CollisionBox(
          Hash name,
          geometry::Vector<T> position,
          geometry::Vector<T> size
        );

        /** Default constructor. */
        CollisionBox();

        /** A collection of named collision boxes. */
        using List = VectorAllocatorList<CollisionBox<T>>;

        /** A name for this collision box. */
        Hash name;
      };

      /** Animation tile data class. */
      class AnimationTile {
      public:

        /** Read serialized animation data from stream. */
        AnimationTile(std::istream& stream);

        /** The animation tile index. */
        uint16_t tile_index;

        /** The duration of the animation tile in frames. */
        uint16_t duration;
      };

      /** Instance constructor. */
      Tile();

      /** Read serialized tile data from stream. */
      void read(std::istream& stream);

      /**
       * The tile data name.
       *
       * This field is not used by ULTRA240 and is intended for the
       * application's own notation.
       */
      Hash name;

      /**
       * A map of collision box lists, the key to each list being the collision
       * box type.
       */
      HashMap<CollisionBox<uint16_t>::List> collision_boxes;

      /** Collection of animation tiles. */
      std::vector<AnimationTile> animation_tiles;

      /** Duration of total animation. */
      uint32_t animation_duration;

      /** Code library associated with this tile. */
      std::unique_ptr<DynamicLibrary> library;
    };

    /** Read a serialized tileset from a file of specified name. */
    Tileset(const std::string& name);

    /** Read a serialized tileset from a stream. */
    Tileset(std::istream& stream);

    /** Return the tile index for a specified name. */
    uint16_t get_tile_index_by_name(uint32_t name) const;

    /** Get number of collision boxes of the specified type. */
    bool get_collision_boxes_count(
      uint16_t tile_index,
      Hash type
    ) const;

    /** Return collision boxes of a specified type. */
    template <typename T>
    void get_collision_boxes(
      Tile::CollisionBox<T>* collision_boxes,
      uint16_t tile_index,
      Hash type,
      geometry::Vector<float> pos = {0, 0},
      Attributes attributes = {
        .flip_x = false,
        .flip_y = false,
      }
    ) const;

    /** 
     * Return a collision box adjusted by the specified position and
     * attributes.
     */
    Tile::CollisionBox<float> adjust_collision_box(
      const Tile::CollisionBox<uint16_t>& collision_box,
      geometry::Vector<float> pos = {0, 0},
      Attributes attributes = {
        .flip_x = false,
        .flip_y = false,
      }
    ) const;

    /** Dimensions of each tile in the tilesheet. */
    geometry::Vector<uint16_t> tile_size;

    /** Collection of tile data. */
    std::vector<Tile> tiles;

    /** Name of bitmap used for rendering. */
    std::string source;

    /** Code library associated with this tileset. */
    std::unique_ptr<DynamicLibrary> library;

  private:

    std::map<uint32_t, uint16_t> name_map;
  };

  /** 
   * Specialization constructor for collision boxes read from serialized tileset
   * data.
   */
  template <>
  Tileset::Tile::CollisionBox<uint16_t>::CollisionBox(
    Hash name,
    std::istream& stream
  );

  /**
   * Specialization constructor for collision boxes created from floating point
   * positions.
   */
  template <>
  Tileset::Tile::CollisionBox<float>::CollisionBox(
    Hash name,
    geometry::Vector<float> position,
    geometry::Vector<float> size
  );

}
