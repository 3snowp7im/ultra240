#include <ultra240/config.h>
#include <ultra240/file.h>
#include <yaml-cpp/yaml.h>

namespace ultra::yaml::config {

  using namespace ultra::config;

  class NodeImpl : public Node::Impl {
  public:

    static NodeImpl* deref(std::unique_ptr<Node::Impl>& impl) {
      return reinterpret_cast<NodeImpl*>(impl.get());
    }

    static const NodeImpl* deref(const std::unique_ptr<Node::Impl>& impl) {
      return reinterpret_cast<const NodeImpl*>(impl.get());
    }

    NodeImpl(std::istream& stream)
      : node(YAML::Load(stream)) {}

    NodeImpl(YAML::Node node)
      : node(node) {}

    NodeImpl()
      : node(YAML::NodeType::value::Map) {}

    ~NodeImpl() {}

    YAML::Node node;
  };

}

namespace ultra::config {

  using namespace ultra::yaml::config;

  void init() {}

  void quit() {}

  Node::Node()
    : impl(new NodeImpl()) {}

  Node::Node(const char* path) {
    file::Input file(path);
    impl.reset(new NodeImpl(file.stream()));
    file.close();
  }

  Node::Node(Node& rhs)
    : impl(new NodeImpl(NodeImpl::deref(rhs.impl)->node)) {}

  Node::Node(Node::Impl* impl)
    : impl(impl) {}

  Node::~Node() {}

  void Node::save(std::ostream& stream) {
    auto impl = NodeImpl::deref(this->impl);
    YAML::Emitter out;
    out << impl->node;
    stream << out.c_str();
  }

  void Node::remove(const char* key) {
    auto impl = NodeImpl::deref(this->impl);
    impl->node.remove(key);
  }

  bool Node::is_defined() const {
    auto impl = NodeImpl::deref(this->impl);
    return impl->node.IsDefined();
  }

  std::string Node::to_string() const {
    auto impl = NodeImpl::deref(this->impl);
    return impl->node.as<std::string>();
  }

  int Node::to_int() const {
    auto impl = NodeImpl::deref(this->impl);
    return impl->node.as<int>();
  }

  bool Node::to_bool() const {
    auto impl = NodeImpl::deref(this->impl);
    return impl->node.as<bool>();
  }

  Node Node::operator[](const char* key) {
    auto impl = NodeImpl::deref(this->impl);
    return Node(new NodeImpl(impl->node[key]));
  }

  Node& Node::operator=(Node rhs) {
    auto impl = NodeImpl::deref(this->impl);
    impl->node = NodeImpl::deref(rhs.impl)->node;
    return *this;
  }

  Node& Node::operator=(std::string rhs) {
    auto impl = NodeImpl::deref(this->impl);
    impl->node = rhs;
    return *this;
  }

  Node& Node::operator=(int rhs) {
    auto impl = NodeImpl::deref(this->impl);
    impl->node = rhs;
    return *this;
  }

  Node& Node::operator=(bool rhs) {
    auto impl = NodeImpl::deref(this->impl);
    impl->node = rhs;
    return *this;
  }

}
