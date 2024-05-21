#include <fstream>
#include "ultra/ultra.h"

namespace ultra {

  Image::Image(const std::string& name) {
    std::string file_name = std::string(name) + ".bmp";
    std::ifstream stream(ultra::path_manager::data_dir + "/img/" + file_name);
    // Read width and height.
    stream.seekg(18);
    size.x = util::read<uint32_t>(stream);
    size.y = util::read<uint32_t>(stream);
    // Read pixel data.
    data.resize(size.x * size.y * sizeof(uint32_t));
    stream.seekg(122);
    util::read<uint32_t>(&data[0], stream, size.x * size.y);
    stream.close();
  }

}
