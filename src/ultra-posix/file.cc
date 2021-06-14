#include <fstream>
#include <ultra240/file.h>
#include <stdexcept>

namespace ultra::posix::file {

  class Input : public ultra::file::Input {
    std::ifstream file;
  public:
    Input(const char* path);
    ~Input();
    std::istream& istream();
    void close();
  };

  Input::Input(const char* path) : file(path) {
    if (!file.is_open()) {
      throw std::runtime_error("Input: could not open " + std::string(path));
    }
  }

  Input::~Input() {
    if (file.is_open()) {
      close();
    }
  }

  std::istream& Input::istream() {
    return file;
  }

  void Input::close() {
    file.close();
  }

  class Output : public ultra::file::Output {
    std::ofstream file;
  public:
    Output(const char* path);
    ~Output();
    std::ostream& ostream();
    void close();
  };

  Output::Output(const char* path) : file(path) {
    if (!file.is_open()) {
      throw std::runtime_error("Output: could not open " + std::string(path));
    }
  }

  Output::~Output() {
    if (file.is_open()) {
      close();
    }
  }

  std::ostream& Output::ostream() {
    return file;
  }

  void Output::close() {
    file.close();
  }

}

namespace ultra::file {

  void init() {
  }

  void quit() {}

  Input* Input::open(const char* path) {
    return new posix::file::Input(path);
  }

  Output* Output::open(const char* path) {
    return new posix::file::Output(path);
  }

}
