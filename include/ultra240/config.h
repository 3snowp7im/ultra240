#pragma once

#include <istream>
#include <memory>
#include <string>

namespace ultra::config {

  void init();

  void quit();

  class Node {
  public:

    Node();

    Node(const char* path);

    Node(Node& rhs);

    ~Node();

    void save(std::ostream& stream);

    Node operator[](const char* key);

    Node& operator=(Node rhs);

    Node& operator=(std::string rhs);

    Node& operator=(int rhs);

    Node& operator=(bool rhs);

    void remove(const char* key);

    bool is_defined() const;

    std::string to_string() const;

    int to_int() const;

    bool to_bool() const;

    class Impl {
    public:
      virtual ~Impl() {};
    };

  private:

    Node(Impl* impl);

    std::unique_ptr<Impl> impl;
  };

}
