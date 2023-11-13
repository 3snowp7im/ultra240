#pragma once

#include <istream>
#include <memory>
#include <ostream>

namespace ultra::file {

  /** Perform module initialization. Must be called before instantiation. */
  void init();

  /** Free module resources. */
  void quit();

  /** An input file. */
  class Input {
  public:

    /** Open a file at the specified path for reading. */
    Input(const char* path);

    /** Instance destructor. */
    ~Input();

    /**
     * Return an istream for the opened file.
     *
     * Reading from a stream of a closed file results in undefined behaviour.
     */
    std::istream& stream();

    /** Close the file. */
    void close();

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    std::unique_ptr<Impl> impl;
  };

  /** An output file. */
  class Output {
  public:

    /** Open a file at the specified path for writing. */
    Output(const char* path);

    /** Instance destructor. */
    ~Output();

    /**
     * Return an ostream for the opened file.
     *
     * Writing to an stream of a closed file results in undefined behaviour.
     */
    std::ostream& stream();

    /** Close the file. */
    void close();

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    std::unique_ptr<Impl> impl;
  };

}
