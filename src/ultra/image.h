#pragma once

#include <vector>
#include <ultra240/geometry.h>

namespace ultra {

  /** Bitmap image class. */
  class Image {
  public:

    /** Load a bitmap from a specified name. */
    Image(const std::string& name);

    /** Image width and height. */
    geometry::Vector<uint32_t> size;

    /** Image pixels. */
    std::vector<uint32_t> data;
  };

}
