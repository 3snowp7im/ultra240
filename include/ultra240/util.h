#pragma once

#include <istream>
#include <ultra240/dynamic_library.h>
#include <ultra240/geometry.h>
#include <ultra240/hash.h>
#include <ultra240/resource_loader.h>
#include <ultra240/tileset.h>
#include <vector>

namespace ultra::util {

  struct Image {
    geometry::Vector<uint32_t> size;
    std::vector<uint32_t> data;
  };

  Image load_image(std::istream& stream);

  Image load_image(ResourceLoader& loader, const char* name);

  DynamicLibrary* load_library(ResourceLoader& loader, const char* name);

  Tileset* load_tileset(ResourceLoader& loader, const char* name);

  template<typename T = int32_t>
  T read_signed_bits(uint32_t& field, uint8_t count) {
    uint32_t mask = (1 << count) - 1;
    uint32_t bits = field & mask;
    if (bits & (1 << (count - 1))) {
      bits |= ~0 ^ ~mask;
    }
    field >>= count;
    return static_cast<T>(bits);
  }

  template<typename T = uint32_t>
  T read_unsigned_bits(uint32_t& field, uint8_t count) {
    uint32_t mask = (1 << count) - 1;
    uint32_t bits = field & mask;
    field >>= count;
    return static_cast<T>(bits);
  }

}
