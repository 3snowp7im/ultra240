#include <fstream>
#include <ultra240/tileset.h>
#include "ultra/ultra.h"

namespace ultra {

  static void read(
    Tileset& ts,
    std::map<uint32_t, uint16_t>& name_map,
    std::unique_ptr<DynamicLibrary>& library,
    std::istream& stream
  ) {
    // Read tile count.
    uint16_t tile_count = util::read<uint16_t>(stream);
    // Read width and height.
    ts.tile_size.x = util::read<uint16_t>(stream);
    ts.tile_size.y = util::read<uint16_t>(stream);
    // Read image source offset.
    uint32_t source_offset = util::read<uint32_t>(stream);
    // Read library name offset.
    uint32_t library_offset = util::read<uint32_t>(stream);
    // Read number of tile data entries.
    uint16_t tile_data_count = util::read<uint16_t>(stream);
    // Read tile offsets.
    uint32_t tile_offsets[tile_data_count];
    for (int i = 0; i < tile_data_count; i++) {
      tile_offsets[i] = util::read<uint32_t>(stream);
    }
    // Read image source.
    stream.seekg(source_offset, stream.beg);
    ts.source = util::read_string(stream);
    // Load dynamic library.
    stream.seekg(library_offset, stream.beg);
    auto library_name = util::read_string(stream);
    if (library_name.size()) {
      library.reset(new dynamic_library::Impl(library_name.c_str()));
    }
    // Read tiles.
    ts.tiles.resize(tile_count);
    for (int i = 0; i < tile_data_count; i++) {
      stream.seekg(tile_offsets[i], stream.beg);
      uint16_t tile_index = util::read<uint16_t>(stream);
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
    name = util::read<uint32_t>(stream);
    // Read library offset.
    uint32_t library_offset = util::read<uint32_t>(stream);
    // Read collision box type count.
    uint16_t collision_box_type_count = util::read<uint16_t>(stream);
    // Read collision box offset.
    std::vector<uint32_t> collision_box_type_offsets(collision_box_type_count);
    for (int i = 0; i < collision_box_type_count; i++) {
      collision_box_type_offsets[i] = util::read<uint32_t>(stream);
    }
    // Read animation tile count.
    uint8_t animation_tile_count = util::read<uint8_t>(stream);
    // Read animation tiles.
    animation_tiles.reserve(animation_tile_count);
    for (int i = 0; i < animation_tile_count; i++) {
      animation_tiles.emplace_back(stream);
      animation_duration += animation_tiles.back().duration;
    }
    // Load dynamic library.
    stream.seekg(library_offset, stream.beg);
    auto library_name = util::read_string(stream);
    if (library_name.size()) {
      library.reset(new dynamic_library::Impl(library_name.c_str()));
    }
    // Load collision boxes.
    for (auto type_offset : collision_box_type_offsets) {
      stream.seekg(type_offset, stream.beg);
      Hash type = util::read<Hash>(stream);
      uint16_t collision_box_list_count  = util::read<uint16_t>(stream);
      std::vector<uint32_t> collision_box_list_offsets(
        collision_box_list_count
      );
      for (int i = 0; i < collision_box_list_count; i++) {
        collision_box_list_offsets[i] = util::read<uint32_t>(stream);
      }
      size_t box_count = 0;
      for (auto list_offset : collision_box_list_offsets) {
        stream.seekg(list_offset);
        util::read<Hash>(stream);
        box_count += util::read<uint16_t>(stream);
      }
      auto& named_list = collision_boxes.emplace(
        type,
        CollisionBox<uint16_t>::List(
          VectorAllocator<CollisionBox<uint16_t>>(box_count)
        )
      ).first->second;
      auto it = named_list.begin();
      for (auto list_offset : collision_box_list_offsets) {
        stream.seekg(list_offset);
        Hash name = util::read<Hash>(stream);
        uint16_t count = util::read<uint16_t>(stream);
        for (int i = 0; i < count; i++) {
          named_list.emplace_back(CollisionBox<uint16_t>(name, stream));
        }
      }
    }
  }

  Tileset::Tile::CollisionBox<float> Tileset::adjust_collision_box(
    const Tile::CollisionBox<uint16_t>& box,
    geometry::Vector<float> pos,
    Attributes attributes
  ) const {
    if (attributes.flip_x && attributes.flip_y) {
      return Tile::CollisionBox<float>(
        box.name,
        pos
        + tile_size.as_x().as<float>()
        - box.position.as<float>()
        - box.size.as<float>(),
        box.size
      );
    } else if (attributes.flip_y) {
      return Tile::CollisionBox<float>(
        box.name,
        pos
        + box.position.as_x().as<float>()
        - box.position.as_y().as<float>()
        - box.size.as_y().as<float>(),
        box.size
      );
    } else if (attributes.flip_x) {
      return Tile::CollisionBox<float>(
        box.name,
        pos
        + tile_size.as_x().as<float>()
        - tile_size.as_y().as<float>()
        - box.position.as_x().as<float>()
        + box.position.as_y().as<float>()
        - box.size.as_x().as<float>(),
        box.size
      );
    }
    return Tile::CollisionBox<float>(
      box.name,
      pos
      - tile_size.as_y().as<float>()
      + box.position.as<float>(),
      box.size
    );
  }

  bool Tileset::get_collision_boxes_count(
    uint16_t tile_index,
    Hash type
  ) const {
    auto it = tiles[tile_index].collision_boxes.find(type);
    if (it == tiles[tile_index].collision_boxes.end()) {
      return 0;
    }
    return it->second.size();
  }

  template <>
  void Tileset::get_collision_boxes<float>(
    Tile::CollisionBox<float>* collision_boxes,
    uint16_t tile_index,
    Hash type,
    geometry::Vector<float> pos,
    Attributes attributes
  ) const {
    auto it = tiles[tile_index].collision_boxes.find(type);
    if (it == tiles[tile_index].collision_boxes.end()) {
      return;
    }
    for (const auto& box : it->second) {
      *collision_boxes++ = adjust_collision_box(box, pos, attributes);
    }
  }

  template <>
  void Tileset::get_collision_boxes<uint16_t>(
    Tile::CollisionBox<uint16_t>* collision_boxes,
    uint16_t tile_index,
    Hash type,
    geometry::Vector<float> pos,
    Attributes attributes
  ) const {
    auto it = tiles[tile_index].collision_boxes.find(type);
    if (it == tiles[tile_index].collision_boxes.end()) {
      return;
    }
    for (const auto& box : it->second) {
      *collision_boxes++ = box;
    }
  }

  template <>
  Tileset::Tile::CollisionBox<uint16_t>::CollisionBox(
    Hash name,
    std::istream& stream
  ) : name(name),
      geometry::Rectangle<uint16_t>(util::read_rectangle<uint16_t>(stream)) {}

  template <>
  Tileset::Tile::CollisionBox<uint16_t>::CollisionBox()
    : geometry::Rectangle<uint16_t>({0, 0}, {0, 0}) {}

  template <>
  Tileset::Tile::CollisionBox<float>::CollisionBox(
    Hash name,
    geometry::Vector<float> position,
    geometry::Vector<float> size
  ) : name(name),
      geometry::Rectangle<float>(position, size) {}

  template <>
  Tileset::Tile::CollisionBox<float>::CollisionBox()
    : geometry::Rectangle<float>({0, 0}, {0, 0}) {}

  Tileset::Tile::AnimationTile::AnimationTile(std::istream& stream)
    : tile_index(util::read<uint16_t>(stream)),
      duration(util::read<uint16_t>(stream)) {}

  uint16_t Tileset::get_tile_index_by_name(uint32_t name) const {
    return name_map.at(name);
  }


}
