#include <vector>
#include "ultra/ultra.h"

namespace ultra::util {

  std::string read_string(std::istream& stream) {
    std::vector<char> buf;
    buf.reserve(256);
    char c;
    do {
      c = util::read<char>(stream);
      buf.push_back(c);
    } while (c != '\0');
    return std::string(&buf[0]);
  }

}
