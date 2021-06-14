#include <stdexcept>
#include <string>
#include <errno.h>
#include "mkdirp.h"

namespace ultra::posix {

  void mkdirp(const char* path, mode_t mode) {
    if(path[0] == '\0') {
      throw std::runtime_error("mkdirp: invalid path");
    }
    std::string copy(path);
    size_t i = 0;
    // Loop through / separated directories.
    while (copy[i] != '\0') {
      // Find first slash or end.
      do {
        i++;
      } while (copy[i] != '\0' && copy[i] != '/');
      // Remember value from i.
      char v = copy[i];
      // Write end of string at i.
      copy[i] = '\0';
      // Create folder from path to '\0' inserted at i.
      if (mkdir(copy.c_str(), mode) != 0 && errno != EEXIST) {
        throw std::runtime_error("mkdirp: could not create directory");
      }
      // Restore path to its former glory.
      copy[i] = v;
    }
  }

}
