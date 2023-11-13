#include <fstream>
#include <ultra240/file.h>
#include <stdexcept>

namespace ultra::posix::file {

  using namespace ultra::file;

  class InputImpl : public Input::Impl {
  public:

    static InputImpl* deref(std::unique_ptr<Input::Impl>& impl) {
      return reinterpret_cast<InputImpl*>(impl.get());
    }

    InputImpl(const char* path)
      : stream(path) {
      if (!stream.is_open()) {
        throw std::runtime_error("Input: could not open " + std::string(path));
      }
    }

    ~InputImpl() {
      if (stream.is_open()) {
        stream.close();
      }
    }

    std::ifstream stream;
  };

  class OutputImpl : public Output::Impl {
  public:

    static OutputImpl*
    deref(std::unique_ptr<Output::Impl>& impl) {
      return reinterpret_cast<OutputImpl*>(impl.get());
    }

    OutputImpl(const char* path)
      : stream(path) {
      if (!stream.is_open()) {
        throw std::runtime_error("Output: could not open " + std::string(path));
      }
    }

    ~OutputImpl() {
      if (stream.is_open()) {
        stream.close();
      }
    }

    std::ofstream stream;
  };

}

namespace ultra::file {

  using namespace ultra::posix::file;

  void init() {}

  void quit() {}

  Input::Input(const char* path)
    : impl(new InputImpl(path)) {}

  Input::~Input() {}

  std::istream& Input::stream() {
    auto impl = InputImpl::deref(this->impl);
    return impl->stream;
  }

  void Input::close() {
    impl = nullptr;
  }

  Output::Output(const char* path)
    : impl(new OutputImpl(path)) {}

  Output::~Output() {}

  std::ostream& Output::stream() {
    auto impl = OutputImpl::deref(this->impl);
    return impl->stream;
  }

  void Output::close() {
    impl = nullptr;
  }

}
