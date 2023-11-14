#include <ultra240/tileset.h>
#include <ultra240/util.h>

namespace ultra {

  Tileset::Tileset() {}

  Tileset::Tileset(PathManager& pm, const char* name) {
    std::string file_name = "tileset/" + std::string(name) + ".bin";
    file::Input file(pm.get_data_path(file_name.c_str()).c_str());
    auto& stream = file.stream();
    read(pm, stream);
    file.close();
  }

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

  void Tileset::read(PathManager& pm, std::istream& stream) {
    // Read tile count.
    uint16_t tile_count;
    stream.read(reinterpret_cast<char*>(&tile_count), sizeof(uint16_t));
    // Read width and height.
    stream.read(reinterpret_cast<char*>(&tile_size.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&tile_size.y), sizeof(uint16_t));
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
      stream.read(
        reinterpret_cast<char*>(&tile_offsets[i]),
        sizeof(uint32_t)
      );
    }
    // Read image source.
    stream.seekg(source_offset, stream.beg);
    source = read_string(stream);
    // Load dynamic library.
    stream.seekg(library_offset, stream.beg);
    auto library_name = read_string(stream);
    if (library_name.size()) {
      library = std::make_shared<DynamicLibrary>(pm, library_name.c_str());
    }
    // Read tiles.
    tiles.resize(tile_count);
    for (int i = 0; i < tile_data_count; i++) {
      stream.seekg(tile_offsets[i], stream.beg);
      uint16_t tile_index;
      stream.read(reinterpret_cast<char*>(&tile_index), sizeof(uint16_t));
      tiles[tile_index].read(pm, stream);
      name_map.insert({tiles[tile_index].name, tile_index});
    }
  }

  void Tileset::Tile::read(PathManager& pm, std::istream& stream) {
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
    animation_tiles.resize(animation_tile_count);
    for (int i = 0; i < animation_tile_count; i++) {
      animation_tiles[i].read(stream);
    }
    // Load dynamic library.
    stream.seekg(library_offset, stream.beg);
    auto library_name = read_string(stream);
    if (library_name.size()) {
      library = std::make_shared<DynamicLibrary>(pm, library_name.c_str());
    }
    // Load collision boxes.
    for (auto type_offset : collision_box_type_offsets) {
      stream.seekg(type_offset, stream.beg);
      Hash<>::Type type;
      stream.read(reinterpret_cast<char*>(&type), sizeof(Hash<>::Type));
      uint16_t collision_box_list_count;
      stream.read(
        reinterpret_cast<char*>(&collision_box_list_count),
        sizeof(uint16_t)
      );
      std::vector<uint32_t>
        collision_box_list_offsets(collision_box_list_count);
      for (int i = 0; i < collision_box_list_count; i++) {
        stream.read(
          reinterpret_cast<char*>(&collision_box_list_offsets[i]),
          sizeof(uint32_t)
        );
      }
      size_t box_count = 0;
      for (auto list_offset : collision_box_list_offsets) {
        stream.seekg(list_offset);
        Hash<>::Type name;
        stream.read(reinterpret_cast<char*>(&name), sizeof(Hash<>::Type));
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
          VectorAllocator<std::pair<Hash<>::Type, CollisionBox>>(box_count)
        )
      ).first->second;
      named_list.resize(box_count);
      auto it = named_list.begin();
      for (auto list_offset : collision_box_list_offsets) {
        stream.seekg(list_offset);
        Hash<>::Type name;
        stream.read(reinterpret_cast<char*>(&name), sizeof(Hash<>::Type));
        uint16_t count;
        stream.read(
          reinterpret_cast<char*>(&count),
          sizeof(uint16_t)
        );
        for (int i = 0; i < count; i++) {
          it->first = name;
          it->second.read(stream);
          it++;
        }
      }
    }
  }

  Tileset::Tile::CollisionBox::CollisionBox()
    : geometry::Rectangle<uint16_t>({0, 0}, {0, 0}) {}

  void Tileset::Tile::CollisionBox::read(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&position.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&position.y), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&size.x), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&size.y), sizeof(uint16_t));
  }

  Tileset::Tile::AnimationTile::AnimationTile() {}

  void Tileset::Tile::AnimationTile::read(std::istream& stream) {
    stream.read(reinterpret_cast<char*>(&tile_index), sizeof(uint16_t));
    stream.read(reinterpret_cast<char*>(&duration), sizeof(uint16_t));
  }

  uint16_t Tileset::get_tile_index_by_name(uint32_t name) const {
    return name_map.at(name);
  }


}
