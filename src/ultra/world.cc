#include <fstream>
#include <unordered_map>
#include <memory>
#include <ultra240/world.h>
#include "ultra/ultra.h"

namespace ultra {

  World::World(const std::string& name) {
    auto path = ultra::path_manager::data_dir + "/world/" + name + ".bin";
    std::ifstream stream(path);
    // Read number of maps.
    uint16_t map_count;
    stream.read(reinterpret_cast<char*>(&map_count), sizeof(uint16_t));
    // Read map header offsets.
    uint32_t map_offsets[map_count];
    for (int i = 0; i < map_count; i++) {
      stream.read(
        reinterpret_cast<char*>(&map_offsets[i]),
        sizeof(uint32_t)
      );
    }
    // Read number of boundaries.
    uint16_t boundaries_count;
    stream.read(reinterpret_cast<char*>(&boundaries_count), sizeof(uint16_t));
    // Read boundary offsets.
    uint32_t boundary_offsets[boundaries_count];
    for (int i = 0; i < boundaries_count; i++) {
      stream.read(
        reinterpret_cast<char*>(&boundary_offsets[i]),
        sizeof(uint32_t)
      );
    }
    // Read maps.
    maps.reserve(map_count);
    for (int i = 0; i < map_count; i++) {
      stream.seekg(map_offsets[i], stream.beg);
      maps.emplace_back(stream);
    }
    // Read points in boundaries.
    std::vector<std::vector<geometry::Vector<int32_t>>> points(
      boundaries_count
    );
    std::vector<uint8_t> boundary_flags(boundaries_count);
    size_t lines_count = 0;
    for (int i = 0; i < boundaries_count; i++) {
      stream.seekg(boundary_offsets[i], stream.beg);
      stream.read(reinterpret_cast<char*>(&boundary_flags[i]), sizeof(uint8_t));
      uint16_t points_count;
      stream.read(reinterpret_cast<char*>(&points_count), sizeof(uint16_t));
      points[i].resize(points_count);
      lines_count += points_count;
      for (int j = 0; j < points_count; j++) {
        stream.read(reinterpret_cast<char*>(&points[i][j].x), sizeof(int32_t));
        stream.read(reinterpret_cast<char*>(&points[i][j].y), sizeof(int32_t));
      }
    }
    // Create line segments from points lists.
    boundaries.reset(new Boundaries(VectorAllocator<Boundary>(lines_count)));
    for (uint i = 0; i < boundaries_count; i++) {
      auto& boundary = points[i];
      uint8_t flags = boundary_flags[i];
      auto a = boundary.cbegin();
      auto b = std::next(a);
      while (b != boundary.cend()) {
        boundaries->emplace_back(flags, *a, *b);
        a++;
        b++;
      }
    }
  }

  World::Boundary::Boundary()
    : geometry::LineSegment<float>(),
      flags(0) {}

  World::Boundary::Boundary(
    const geometry::Vector<float>& p,
    const geometry::Vector<float>& q
  ) : geometry::LineSegment<float>(p, q),
      flags(0) {}

  World::Boundary::Boundary(
    uint8_t flags,
    const geometry::Vector<float>& p,
    const geometry::Vector<float>& q
  ) : geometry::LineSegment<float>(p, q),
      flags(flags) {}

  const World::Boundaries& World::get_boundaries() const {
    return *boundaries;
  }

  World::Map::Map(std::istream& stream) {
    // Read map position in world.
    stream.read(reinterpret_cast<char*>(&position.x), sizeof(int16_t));
    stream.read(reinterpret_cast<char*>(&position.y), sizeof(int16_t));
    // Read map width and height.
    stream.read(reinterpret_cast<char*>(&size.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&size.y), sizeof(uint16_t));
    // Read properties.
    uint8_t properties_count;
    stream.read(reinterpret_cast<char*>(&properties_count), sizeof(uint8_t));
    for (int i = 0; i < properties_count; i++) {
      uint32_t name, value;
      stream.read(reinterpret_cast<char*>(&name), sizeof(Hash<>::Type));
      stream.read(reinterpret_cast<char*>(&value), sizeof(uint32_t));
      properties.emplace(name, value);
    }
    // Read map tileset count.
    uint8_t map_tileset_count;
    stream.read(
      reinterpret_cast<char*>(&map_tileset_count),
      sizeof(uint8_t)
    );
    // Maintain a set of tileset offsets.
    std::map<uint32_t, std::shared_ptr<Tileset>> tilesets;
    // Read map tileset offsets.
    uint32_t map_tileset_offsets[map_tileset_count];
    for (int i = 0; i < map_tileset_count; i++) {
      uint32_t offset;
      stream.read(
        reinterpret_cast<char*>(&offset),
        sizeof(uint32_t)
      );
      tilesets.emplace(offset, nullptr);
      map_tileset_offsets[i] = offset;
    }
    // Read entity tileset count.
    uint8_t entity_tileset_count;
    stream.read(
      reinterpret_cast<char*>(&entity_tileset_count),
      sizeof(uint8_t)
    );
    // Read entity tileset offsets.
    uint32_t entity_tileset_offsets[entity_tileset_count];
    for (int i = 0; i < entity_tileset_count; i++) {
      uint32_t offset;
      stream.read(
        reinterpret_cast<char*>(&offset),
        sizeof(uint32_t)
      );
      tilesets.emplace(offset, nullptr);
      entity_tileset_offsets[i] = offset;
    }
    // Read layer count.
    uint8_t layer_count;
    stream.read(reinterpret_cast<char*>(&layer_count), sizeof(uint8_t));
    // Read layer offsets.
    uint32_t layer_offsets[layer_count];
    for (int i = 0; i < layer_count; i++) {
      stream.read(
        reinterpret_cast<char*>(&layer_offsets[i]),
        sizeof(uint32_t)
      );
    }
    // Read entity count.
    uint16_t entity_count;
    stream.read(reinterpret_cast<char*>(&entity_count), sizeof(uint16_t));
    // Read entities.
    entities.reserve(entity_count);
    for (int i = 0; i < entity_count; i++) {
      entities.emplace(entities.end(), stream);
    }
    sorted_entities.x.min.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      uint16_t index;
      stream.read(reinterpret_cast<char*>(&index), sizeof(uint16_t));
      sorted_entities.x.min[i] = index;
    }
    sorted_entities.x.max.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      uint16_t index;
      stream.read(reinterpret_cast<char*>(&index), sizeof(uint16_t));
      sorted_entities.x.max[i] = index;
    }
    sorted_entities.y.min.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      uint16_t index;
      stream.read(reinterpret_cast<char*>(&index), sizeof(uint16_t));
      sorted_entities.y.min[i] = index;
    }
    sorted_entities.y.max.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      uint16_t index;
      stream.read(reinterpret_cast<char*>(&index), sizeof(uint16_t));
      sorted_entities.y.max[i] = index;
    }
    // Read map tilesets.
    map_tilesets.resize(map_tileset_count);
    for (int i = 0; i < map_tileset_count; i++) {
      stream.seekg(map_tileset_offsets[i], stream.beg);
      tilesets.at(map_tileset_offsets[i]).reset(new Tileset(stream));
      map_tilesets[i] = tilesets.at(map_tileset_offsets[i]);
    }
    // Read entity tilesets.
    entity_tilesets.resize(entity_tileset_count);
    for (int i = 0; i < entity_tileset_count; i++) {
      stream.seekg(entity_tileset_offsets[i], stream.beg);
      if (tilesets.at(entity_tileset_offsets[i]) == nullptr) {
        tilesets.at(entity_tileset_offsets[i]).reset(new Tileset(stream));
      }
      entity_tilesets[i] = tilesets.at(entity_tileset_offsets[i]);
    }
    // Link entities with their tilesets.
    for (auto& entity : entities) {
      entity.tileset = entity_tilesets[entity.tile_index >> 12].get();
      entity.attributes.flip_x = entity.tile_index & 0x800;
      entity.attributes.flip_y = entity.tile_index & 0x400;
      entity.tile_index = (entity.tile_index & 0x3ff) - 1;
    }
    // Read layers.
    size_t area = size.x * size.y;
    tiles.resize(layer_count * area);
    layers.reserve(layer_count);
    for (int i = 0; i < layer_count; i++) {
      // Read layer.
      stream.seekg(layer_offsets[i], stream.beg);
      layers.emplace_back(stream);
      // Read tiles.
      stream.read(
        reinterpret_cast<char*>(&tiles[i * area]),
        sizeof(uint16_t) * area
      );
    }
  }

  World::Map::Layer::Layer(std::istream& stream) {
    // Read name.
    stream.read(reinterpret_cast<char*>(&name), sizeof(uint32_t));
    // Read parallax.
    uint8_t pn, pd;
    stream.read(reinterpret_cast<char*>(&pn), sizeof(uint8_t));
    stream.read(reinterpret_cast<char*>(&pd), sizeof(uint8_t));
    parallax.x = static_cast<float>(pn) / pd;
    stream.read(reinterpret_cast<char*>(&pn), sizeof(uint8_t));
    stream.read(reinterpret_cast<char*>(&pd), sizeof(uint8_t));
    parallax.y = static_cast<float>(pn) / pd;
  }

  World::Map::Entity::Entity(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&layer_name), sizeof(uint32_t));
    stream.read(reinterpret_cast<char*>(&position.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&position.y), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&tile_index), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&type), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&id), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&state), sizeof(uint32_t));
  }

}
