#pragma once

#include <istream>
#include <map>
#include <memory>
#include <ultra240/hash.h>
#include <ultra240/dynamic_library.h>
#include <ultra240/geometry.h>
#include <ultra240/resource_loader.h>
#include <ultra240/vector_allocator.h>
#include <string>
#include <vector>

namespace ultra {

  class Tileset {
  public:

    class Tile {
    public:

      class CollisionBox : public geometry::Rectangle<uint16_t> {
      public:

        CollisionBox();

        void read(std::istream& stream);

        using NamedList = VectorAllocatorList<
          std::pair<Hash<>::Type, CollisionBox>
        >;
      };

      class AnimationTile {
      public:

        AnimationTile();

        void read(std::istream& stream);

        uint16_t tile_index;

        uint16_t duration;
      };

      void read(ResourceLoader& loader, std::istream& stream);

      Hash<>::Type name;

      HashMap<CollisionBox::NamedList> collision_boxes;

      std::vector<AnimationTile> animation_tiles;

      std::shared_ptr<DynamicLibrary> library;
    };

    Tileset();

    void read(ResourceLoader& loader, std::istream& stream);

    uint16_t get_tile_index_by_name(uint32_t name) const;

    geometry::Vector<uint16_t> tile_size;

    std::vector<Tile> tiles;

    std::string source;

    std::shared_ptr<DynamicLibrary> library;

  private:

    std::map<uint32_t, uint16_t> name_map;
  };

}
