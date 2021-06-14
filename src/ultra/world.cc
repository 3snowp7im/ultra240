#include <unordered_map>
#include <memory>
#include <ultra240/file.h>
#include <ultra240/world.h>

namespace ultra {

  World::World(ResourceLoader& loader, const char* name) {
    auto filename = std::string("world/") + name + ".bin";
    std::shared_ptr<file::Input> file(loader.open_data(filename.c_str()));
    auto& stream = file->istream();
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
    maps.resize(map_count);
    for (int i = 0; i < map_count; i++) {
      stream.seekg(map_offsets[i], stream.beg);
      maps[i].read(loader, stream);
    }
    // Read points in boundaries.
    std::vector<std::vector<geometry::Vector<int32_t>>> points(
      boundaries_count
    );
    size_t lines_count = 0;
    for (int i = 0; i < boundaries_count; i++) {
      stream.seekg(boundary_offsets[i], stream.beg);
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
    boundaries.reset(
      new Boundaries(
        VectorAllocator<geometry::LineSegment<float>>(lines_count)
      )
    );
    for (const auto& boundary : points) {
      auto a = boundary.cbegin();
      auto b = std::next(a);
      while (a != boundary.cend()) {
        boundaries->emplace_back(a->as<float>(), b->as<float>());
        a++;
        b++;
        if (b == boundary.cend()) {
          b = boundary.cbegin();
        }
      }
    }
  }

  const World::Boundaries& World::get_boundaries() const {
    return *boundaries;
  }

  World::Map::Map() {}

  void World::Map::read(ResourceLoader& loader, std::istream& stream) {
    // Read map position in world.
    stream.read(reinterpret_cast<char*>(&position.x), sizeof(int16_t));
    stream.read(reinterpret_cast<char*>(&position.y), sizeof(int16_t));
    // Read map width and height.
    stream.read(reinterpret_cast<char*>(&size.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&size.y), sizeof(uint16_t));
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
    // Read entity layer index.
    stream.read(reinterpret_cast<char*>(&entities_index), sizeof(uint8_t));
    // Read entity count.
    uint16_t entity_count;
    stream.read(reinterpret_cast<char*>(&entity_count), sizeof(uint16_t));
    // Read entities.
    entities.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      entities[i].read(stream);
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
      tilesets.at(map_tileset_offsets[i]).reset(new Tileset);
      tilesets.at(map_tileset_offsets[i])->read(loader, stream);
      map_tilesets[i] = tilesets.at(map_tileset_offsets[i]);
    }
    // Read entity tilesets.
    entity_tilesets.resize(entity_tileset_count);
    for (int i = 0; i < entity_tileset_count; i++) {
      stream.seekg(entity_tileset_offsets[i], stream.beg);
      if (tilesets.at(entity_tileset_offsets[i]) == nullptr) {
        tilesets.at(entity_tileset_offsets[i]).reset(new Tileset);
        tilesets.at(entity_tileset_offsets[i])->read(loader, stream);
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
    layers.resize(layer_count);
    for (int i = 0; i < layer_count; i++) {
      stream.seekg(layer_offsets[i], stream.beg);
      layers[i].read(stream, size);
    }
  }

  World::Map::Layer::Layer() {}

  void World::Map::Layer::read(
    std::istream& stream,
    const geometry::Vector<uint16_t>& size
  ) {
    // Read parallax.
    uint8_t pn, pd;
    stream.read(reinterpret_cast<char*>(&pn), sizeof(uint8_t));
    stream.read(reinterpret_cast<char*>(&pd), sizeof(uint8_t));
    parallax.x = static_cast<float>(pn) / pd;
    stream.read(reinterpret_cast<char*>(&pn), sizeof(uint8_t));
    stream.read(reinterpret_cast<char*>(&pd), sizeof(uint8_t));
    parallax.y = static_cast<float>(pn) / pd;
    // Read tiles.
    tiles.resize(size.x * size.y);
    stream.read(
      reinterpret_cast<char*>(&tiles[0]),
      size.x * size.y * sizeof(uint16_t)
    );
  }

  World::Map::Entity::Entity() {}

  void World::Map::Entity::read(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&position.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&position.y), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&tile_index), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&state), sizeof(uint32_t));
  }

}
