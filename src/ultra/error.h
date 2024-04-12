#include <stdexcept>
#include <string>

namespace ultra {

  inline std::runtime_error error(
    const char* file,
    int line,
    const std::string& error
  ) {
    auto msg = std::string(file) + ":" + std::to_string(line) + ": " + error;
    return std::runtime_error(msg);
  }

}
