#include <ultra240/file.h>
#include <ultra240/image.h>

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

  Image::Image(PathManager& pm, const char* name) {
    std::string file_name = "img/" + std::string(name) + ".bmp";
    file::Input file(pm.get_data_path(file_name.c_str()).c_str());
    read(*this, file.stream());
    file.close();
  }

}
