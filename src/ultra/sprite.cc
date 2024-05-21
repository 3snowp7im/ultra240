#include <ultra240/sprite.h>

namespace ultra {

  Sprite::Sprite(
    const Tileset& tileset,
    const geometry::Vector<float>& position,
    Tileset::Attributes attributes,
    uint16_t tile_index,
    float transform[9]
  ) : tileset(tileset),
      position(position),
      attributes(attributes),
      tile_index(tile_index) {
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
