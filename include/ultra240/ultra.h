#pragma once

#include <string>
#include <ultra240/dynamic_library.h>
#include <ultra240/entity.h>
#include <ultra240/geometry.h>
#include <ultra240/hash.h>
#include <ultra240/renderer.h>
#include <ultra240/tileset.h>
#include <ultra240/util.h>
#include <ultra240/vector_allocator.h>
#include <ultra240/world.h>

namespace ultra {

  /** Initialize library with specified path to data path prefix. */
  void init(const std::string& data_dir);

  /** Free library resources. */
  void quit();

}
