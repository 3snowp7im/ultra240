#pragma once

#include <ultra240/geometry.h>
#include <ultra240/path_manager.h>
#include <vector>

namespace ultra {

  /** Bitmap image class. */
  class Image {
  public:

    /** Load a bitmap from a specified name. */
    Image(PathManager& pm, const char* name);

    /** Image width and height. */
    geometry::Vector<uint32_t> size;

    /** Image pixels. */
    std::vector<uint32_t> data;
  };

}
