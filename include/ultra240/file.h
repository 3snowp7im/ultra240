#pragma once

#include <istream>
#include <ostream>

namespace ultra::file {

  void init();

  void quit();
    
  class Input {
  public:

    static Input* open(const char* path);

    virtual ~Input() {}

    virtual std::istream& istream() = 0;

    virtual void close() = 0;
  };

  class Output {
  public:

    static Output* open(const char* path);

    virtual ~Output() {}

    virtual std::ostream& ostream() = 0;

    virtual void close() = 0;
  };

}
