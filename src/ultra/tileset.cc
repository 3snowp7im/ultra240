#include <fstream>
#include <ultra240/tileset.h>
#include "ultra/ultra.h"

namespace ultra {

  static std::string read_string(std::istream& stream) {
    std::vector<char> buf;
    buf.reserve(256);
    char c;
    do {
      stream.read(&c, sizeof(c));
      buf.push_back(c);
    } while (c != '\0');
    return std::string(&buf[0]);
  }

  static void read(
    Tileset& ts,
    std::map<uint32_t, uint16_t>& name_map,
    std::unique_ptr<DynamicLibrary>& library,
    std::istream& stream
  ) {
    // Read tile count.
    uint16_t tile_count;
    stream.read(reinterpret_cast<char*>(&tile_count), sizeof(uint16_t));
    // Read width and height.
    stream.read(reinterpret_cast<char*>(&ts.tile_size.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&ts.tile_size.y), sizeof(uint16_t));
    // Read image source offset.
    uint32_t source_offset;
    stream.read(reinterpret_cast<char*>(&source_offset), sizeof(uint32_t));
    // Read library name offset.
    uint32_t library_offset;
    stream.read(reinterpret_cast<char*>(&library_offset), sizeof(uint32_t));
    // Read number of tile data entries.
    uint16_t tile_data_count;
    stream.read(reinterpret_cast<char*>(&tile_data_count), sizeof(uint16_t));
    // Read tile offsets.
    uint32_t tile_offsets[tile_data_count];
    for (int i = 0; i < tile_data_count; i++) {
      stream.read(reinterpret_cast<char*>(&tile_offsets[i]), sizeof(uint32_t));
    }
    // Read image source.
    stream.seekg(source_offset, stream.beg);
    ts.source = read_string(stream);
    // Load dynamic library.
    stream.seekg(library_offset, stream.beg);
    auto library_name = read_string(stream);
    if (library_name.size()) {
      library.reset(new dynamic_library::Impl(library_name.c_str()));
    }
    // Read tiles.
    ts.tiles.resize(tile_count);
    for (int i = 0; i < tile_data_count; i++) {
      stream.seekg(tile_offsets[i], stream.beg);
      uint16_t tile_index;
      stream.read(reinterpret_cast<char*>(&tile_index), sizeof(uint16_t));
      ts.tiles[tile_index].read(stream);
      name_map.insert({ts.tiles[tile_index].name, tile_index});
    }
  }

  Tileset::Tileset(const std::string& name) {
    auto path = ultra::path_manager::data_dir + "/tileset/" + name + ".bin";
    std::ifstream file(path);
    read(*this, name_map, library, file);
    file.close();
  }

  Tileset::Tileset(std::istream& stream) {
    read(*this, name_map, library, stream);
  }

  Tileset::Tile::Tile()
    : animation_duration(0) {}

  void Tileset::Tile::read(std::istream& stream) {
    // Read tile name.
    stream.read(reinterpret_cast<char*>(&name), sizeof(uint32_t));
    // Read library offset.
    uint32_t library_offset;
    stream.read(reinterpret_cast<char*>(&library_offset), sizeof(uint32_t));
    // Read collision box type count.
    uint16_t collision_box_type_count;
    stream.read(
      reinterpret_cast<char*>(&collision_box_type_count),
      sizeof(uint16_t)
    );
    // Read collision box offset.
    std::vector<uint32_t> collision_box_type_offsets(collision_box_type_count);
    for (int i = 0; i < collision_box_type_count; i++) {
      stream.read(
        reinterpret_cast<char*>(&collision_box_type_offsets[i]),
        sizeof(uint32_t)
      );
    }
    // Read animation tile count.
    uint8_t animation_tile_count;
    stream.read(
      reinterpret_cast<char*>(&animation_tile_count),
      sizeof(uint8_t)
    );
    // Read animation tiles.
    animation_tiles.reserve(animation_tile_count);
    for (int i = 0; i < animation_tile_count; i++) {
      animation_tiles.emplace_back(stream);
      animation_duration += animation_tiles.back().duration;
    }
    // Load dynamic library.
    stream.seekg(library_offset, stream.beg);
    auto library_name = read_string(stream);
    if (library_name.size()) {
      library.reset(new dynamic_library::Impl(library_name.c_str()));
    }
    // Load collision boxes.
    for (auto type_offset : collision_box_type_offsets) {
      stream.seekg(type_offset, stream.beg);
      Hash type;
      stream.read(reinterpret_cast<char*>(&type), sizeof(Hash));
      uint16_t collision_box_list_count;
      stream.read(
        reinterpret_cast<char*>(&collision_box_list_count),
        sizeof(uint16_t)
      );
      std::vector<uint32_t> collision_box_list_offsets(
        collision_box_list_count
      );
      for (int i = 0; i < collision_box_list_count; i++) {
        stream.read(
          reinterpret_cast<char*>(&collision_box_list_offsets[i]),
          sizeof(uint32_t)
        );
      }
      size_t box_count = 0;
      for (auto list_offset : collision_box_list_offsets) {
        stream.seekg(list_offset);
        Hash name;
        stream.read(reinterpret_cast<char*>(&name), sizeof(Hash));
        uint16_t count;
        stream.read(
          reinterpret_cast<char*>(&count),
          sizeof(uint16_t)
        );
        box_count += count;
      }
      auto& named_list = collision_boxes.emplace(
        type,
        CollisionBox::NamedList(
          VectorAllocator<std::pair<Hash, CollisionBox>>(box_count)
        )
      ).first->second;
      auto it = named_list.begin();
      for (auto list_offset : collision_box_list_offsets) {
        stream.seekg(list_offset);
        Hash name;
        stream.read(reinterpret_cast<char*>(&name), sizeof(Hash));
        uint16_t count;
        stream.read(
          reinterpret_cast<char*>(&count),
          sizeof(uint16_t)
        );
        for (int i = 0; i < count; i++) {
          named_list.emplace_back(std::make_pair(name, CollisionBox(stream)));
        }
      }
    }
  }

  Tileset::Tile::CollisionBox::CollisionBox(std::istream& stream)
    : geometry::Rectangle<uint16_t>({0, 0}, {0, 0}) {
    stream.read(reinterpret_cast<char*>(&position.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&position.y), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&size.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&size.y), sizeof(uint16_t));
  }

  Tileset::Tile::AnimationTile::AnimationTile(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&tile_index), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&duration), sizeof(uint16_t));
  }

  uint16_t Tileset::get_tile_index_by_name(uint32_t name) const {
    return name_map.at(name);
  }


}
