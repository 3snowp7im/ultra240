#pragma once

#include <istream>
#include <memory>
#include <ultra240/geometry.h>
#include <ultra240/resource_loader.h>
#include <ultra240/tileset.h>
#include <ultra240/vector_allocator.h>
#include <vector>

namespace ultra {

  class World {
  public:

    class Map {
    public:

      class Layer {
      public:

        Layer();

        void read(
          std::istream& stream,
          const geometry::Vector<uint16_t>& size
        );

        geometry::Vector<float> parallax;

        std::vector<uint16_t> tiles;
      };

      class Entity {
      public:

        Entity();

        void read(std::istream& stream);

        const Tileset* tileset;

        uint16_t tile_index;

        struct {
          bool flip_x;
          bool flip_y;
        } attributes;

        geometry::Vector<uint32_t> position;

        uint32_t state;
      };

      Map();

      void read(ResourceLoader& loader, std::istream& stream);

      geometry::Vector<int16_t> position;

      geometry::Vector<uint16_t> size;

      uint8_t entities_index;

      std::vector<std::shared_ptr<Tileset>> map_tilesets;

      std::vector<std::shared_ptr<Tileset>> entity_tilesets;

      std::vector<Layer> layers;

      std::vector<Entity> entities;

      struct {
        struct {
          std::vector<uint16_t> min, max;
        } x, y;
      } sorted_entities;
    };

    class Boundary : public geometry::LineSegment<float> {
    public:

      enum Flags {
        OneWay = 0x40,
      };

      Boundary();

      Boundary(
        const geometry::Vector<float>& p,
        const geometry::Vector<float>& q
      );

      Boundary(
        const geometry::Vector<float>& p,
        const geometry::Vector<float>& q,
        uint8_t flags
      );

      uint8_t flags;
    };

    using Boundaries = VectorAllocatorList<Boundary>;

    World(ResourceLoader& loader, const char* name);

    const Boundaries& get_boundaries() const;

    std::vector<Map> maps;

  private:

    std::unique_ptr<Boundaries> boundaries;
  };

}
