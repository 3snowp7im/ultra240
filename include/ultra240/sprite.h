#pragma once

#include <ultra240/geometry.h>
#include <ultra240/tileset.h>

namespace ultra {

  /**
   * Sprite class.
   */
  class Sprite {
  public:

    /** Sprite constructor. */
    Sprite(
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Tileset::Attributes attributes,
      uint16_t tile_index,
      float transform[9] = nullptr
    );

    /** The tileset associated with this sprite. */
    const Tileset& tileset;

    /** The sprite world position. */
    geometry::Vector<float> position;

    /** The sprite attributes. */
    Tileset::Attributes attributes;

    /** Tile index used for rendering. */
    uint16_t tile_index;

    /**
     * Transformation matrix used for rendering.
     *
     * This is initialized to an identity matrix.
     */
    float transform[9];
  };

}
