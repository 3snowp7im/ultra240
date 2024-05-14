#pragma once

#include <ultra240/geometry.h>
#include <ultra240/tileset.h>

namespace ultra {

  /**
   * Sprite class.
   */
  class Sprite {
  public:

    /** Rendering attributes. */
    struct Attributes {
      /** Sprite is rendered flipped about its x-axis. */
      bool flip_x = false;
      /** Sprite is rendered flipped about its y-axis. */
      bool flip_y = false;
    };

    /** Sprite constructor. */
    inline Sprite(
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Attributes attributes,
      uint16_t tile_index,
      float transform[9]
    ) : tileset(tileset),
        position(position),
        attributes(attributes),
        tile_index(tile_index),
        transform(
          transform ? transform : {1, 0, 0, 0, 1, 0, 0, 0, 1}
        ) {}

    /** The tileset associated with this sprite. */
    const Tileset& tileset;

    /** The sprite world position. */
    geometry::Vector<float> position;

    /** The sprite attributes. */
    Attributes attributes;

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
