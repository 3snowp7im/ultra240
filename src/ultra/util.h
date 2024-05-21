#pragma once

#include <istream>
#include <string>
#include <ultra240/geometry.h>

namespace ultra::util {

  template <typename T>
  inline T read(std::istream& stream) {
    T t;
    stream.read(reinterpret_cast<char*>(&t), sizeof(T));
    return t;
  }

  template <typename T>
  inline void read(T* buf, std::istream& stream, size_t count) {
    stream.read(reinterpret_cast<char*>(buf), count * sizeof(T));
  }

  template <typename T>
  inline geometry::Vector<T> read_vector(std::istream& stream) {
    T x = read<T>(stream);
    T y = read<T>(stream);
    return {x, y};
  }

  template <typename T>
  inline geometry::Rectangle<T> read_rectangle(std::istream& stream) {
    geometry::Vector<T> position = read_vector<T>(stream);
    geometry::Vector<T> size = read_vector<T>(stream);
    return {position, size};
  }

  std::string read_string(std::istream& stream);

}
