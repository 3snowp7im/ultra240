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
      class CollisionBox : public geometry::Rectangle<uint16_t> {
      public:

        /** Read serialized collision box from stream. */
        CollisionBox(std::istream& stream);

        /** A collection of named collision boxes. */
        using NamedList = VectorAllocatorList<
          std::pair<Hash<>::Type, CollisionBox>
        >;
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
      Hash<>::Type name;

      /**
       * A map of collision box lists, the key to each list being the collision
       * box type.
       */
      HashMap<CollisionBox::NamedList> collision_boxes;

      /** Collection of animation tiles. */
      std::vector<AnimationTile> animation_tiles;

      /** Code library associated with this tile. */
      std::unique_ptr<DynamicLibrary> library;
    };

    /** Read a serialized tileset from a file of specified name. */
    Tileset(const std::string& name);

    /** Read a serialized tileset from a stream. */
    Tileset(std::istream& stream);

    /** Return the tile index for a specified name. */
    uint16_t get_tile_index_by_name(uint32_t name) const;

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

}
