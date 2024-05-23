#pragma once

#include <ultra240/geometry.h>
#include <ultra240/tileset.h>

namespace ultra {

  /** Base sprite class. */
  class Sprite {
  public:

    /** Sprite constructor. */
    Sprite(
      const Tileset& tileset,
      uint16_t tile_index,
      const geometry::Vector<float>& position,
      Tileset::Attributes attributes,
      float transform[9] = nullptr
    );

    /** The tileset associated with this sprite. */
    const Tileset& tileset;

    /** Tile index used for rendering. */
    uint16_t tile_index;

    /** The sprite world position. */
    geometry::Vector<float> position;

    /** The sprite attributes. */
    Tileset::Attributes attributes;

    /**
     * Transformation matrix used for rendering.
     *
     * This is initialized to an identity matrix.
     */
    float transform[9];
  };

}
