#include <fstream>
#include "ultra.h"
#include "image.h"

namespace ultra {

  static void read(Image& img, std::istream& stream) {
    // Read width and height.
    stream.seekg(18);
    stream.read(reinterpret_cast<char*>(&img.size.x), sizeof(uint32_t));
    stream.read(reinterpret_cast<char*>(&img.size.y), sizeof(uint32_t));
    // Read pixel data.
    size_t data_size = img.size.x * img.size.y * sizeof(uint32_t);
    img.data.resize(data_size);
    stream.seekg(122);
    stream.read(reinterpret_cast<char*>(&img.data[0]), data_size);
  }

  Image::Image(const std::string& name) {
    std::string file_name = std::string(name) + ".bmp";
    std::ifstream file(ultra::data_dir + "/img/" + file_name);
    read(*this, file);
    file.close();
  }

}
