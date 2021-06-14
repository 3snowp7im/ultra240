#include <memory>
#include <ultra240/util.h>

namespace ultra::util {

  Image load_image(std::istream& stream) {
    Image img;
    // Read width and height.
    stream.seekg(18);
    stream.read(reinterpret_cast<char*>(&img.size.x), sizeof(uint32_t));
    stream.read(reinterpret_cast<char*>(&img.size.y), sizeof(uint32_t));
    // Read pixel data.
    size_t data_size = img.size.x * img.size.y * sizeof(uint32_t);
    img.data.resize(data_size);
    stream.seekg(122);
    stream.read(reinterpret_cast<char*>(&img.data[0]), data_size);
    return img;
  }

  Image load_image(ResourceLoader& loader, const char* name) {
    std::string file_name = "img/" + std::string(name) + ".bmp";
    std::shared_ptr<file::Input> img_file(loader.open_data(file_name.c_str()));
    return load_image(img_file->istream());
  }

  Tileset* load_tileset(
    ResourceLoader& loader,
    const char* name
  ) {
    std::string file_name = "tileset/" + std::string(name) + ".bin";
    std::shared_ptr<file::Input> ts_file(loader.open_data(file_name.c_str()));
    auto& stream = ts_file->istream();
    ultra::Tileset* tileset = new ultra::Tileset;
    tileset->read(loader, stream);
    return tileset;
  }

}
