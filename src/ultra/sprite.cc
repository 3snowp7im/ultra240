#include <ultra240/sprite.h>

namespace ultra {

  Sprite::Sprite(
    const Tileset& tileset,
    uint16_t tile_index,
    const geometry::Vector<float>& position,
    Tileset::Attributes attributes,
    float transform[9]
  ) : tileset(tileset),
      tile_index(tile_index),
      position(position),
      attributes(attributes) {
    if (transform) {
      for (int i = 0; i < 9; i++) {
        this->transform[i] = transform[i];
      }
    } else {
      for (int i = 0; i < 9; i++) {
        if (i % 4 == 0) {
          this->transform[i] = 1;
        } else {
          this->transform[i] = 0;
        }
      }
    }
  }

}
