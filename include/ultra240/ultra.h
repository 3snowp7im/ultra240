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

  /**
   * Initialize library with specified name.
   *
   * The name should be chosen so that resource files can be found on the
   * filesystem.
   */
  void init(const std::string& name);

  /** Free library resources. */
  void quit();

}
