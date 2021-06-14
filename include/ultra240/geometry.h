#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace ultra::geometry {

  template <typename T>
  class Vector {
  public:

    static inline Vector<T> from_slope(T slope, T direction) {
      if (slope == std::numeric_limits<T>::infinity()
          || slope == -std::numeric_limits<T>::infinity()) {
        return Vector<T>(0, direction);
      }
      return Vector<T>(1, slope).unit() * direction;
    }

    static Vector<T> NaN() {
      return Vector<T>(
        std::numeric_limits<T>::signaling_NaN(),
        std::numeric_limits<T>::signaling_NaN()
      );
    }

    Vector() : x(0), y(0) {}

    Vector(T x, T y) : x(x), y(y) {}

    template <typename S>
    Vector<S> as() const {
      return {static_cast<S>(x), static_cast<S>(y)};
    }

    template <typename S>
    operator Vector<S>() const {
      return as<S>();
    }

    template <typename S>
    Vector<T>& operator=(S rhs) {
      x = rhs;
      y = rhs;
      return *this;
    }

    Vector<T> operator+(const Vector<T>& rhs) const {
      return {x + rhs.x, y + rhs.y};
    }

    Vector<T>& operator+=(const Vector<T>& rhs) {
      x += rhs.x;
      y += rhs.y;
      return *this;
    }

    Vector<T> operator-(const Vector<T>& rhs) const {
      return {x - rhs.x, y - rhs.y};
    }

    Vector<T>& operator-=(const Vector<T>& rhs) {
      x -= rhs.x;
      y -= rhs.y;
      return *this;
    }

    Vector<T> operator*(const Vector<T>& rhs) const {
      return {x * rhs.x, y * rhs.y};
    }

    Vector<T>& operator*=(const Vector<T>& rhs) {
      x *= rhs.x;
      y *= rhs.y;
      return *this;
    }

    Vector<T> operator/(const Vector<T>& rhs) const {
      return {x / rhs.x, y / rhs.y};
    }

    Vector<T>& operator/=(const Vector<T>& rhs) {
      x /= rhs.x;
      y /= rhs.y;
      return *this;
    }

    bool operator==(const Vector<T>& rhs) const {
      return x == rhs.x && y == rhs.y;
    }

    bool operator!=(const Vector<T>& rhs) {
      return !(*this == rhs);
    }

    Vector<T> operator+(T rhs) const {
      return {x + rhs, y + rhs};
    }

    Vector<T> operator-(T rhs) const {
      return {x - rhs, y - rhs};
    }

    Vector<T> operator*(T rhs) const {
      return {x * rhs, y * rhs};
    }

    Vector<T> operator/(T rhs) const {
      return {x / rhs, y / rhs};
    }

    bool is_nan() const {
      return std::isnan(x) || std::isnan(y);
    }

    Vector<T> as_x() const {
      return {x, 0};
    }

    Vector<T> as_y() const {
      return {0, y};
    }

    T slope() const {
      if (x == 0) {
        return std::numeric_limits<T>::infinity();
      }
      return y / x;
    }

    T length() const {
      return std::sqrt(x * x + y * y);
    }

    T cross(const Vector<T>& rhs) const {
      return x * rhs.y - y * rhs.x;
    }

    T dot(const Vector<T>& rhs) const {
      return x * rhs.x + y * rhs.y;
    }

    Vector<T> unit() const {
      return *this / length();
    }

    std::string to_string() const {
      return "{" + std::to_string(x) + "," + std::to_string(y) + "}";
    }

    T x, y;
  };

  template <typename T>
  Vector<T> operator+(T lhs, const Vector<T>& rhs) {
    return {lhs + rhs.x, lhs + rhs.y};
  }

  template <typename T>
  Vector<T> operator-(T lhs, const Vector<T>& rhs) {
    return {lhs - rhs.x, lhs - rhs.y};
  }

  template <typename T>
  Vector<T> operator*(T lhs, const Vector<T>& rhs) {
    return {lhs * rhs.x, lhs * rhs.y};
  }

  template <typename T>
  Vector<T> operator/(T lhs, const Vector<T>& rhs) {
    return {lhs / rhs.x, lhs / rhs.y};
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator+(const Vector<T>& lhs, S rhs) {
    return {
      static_cast<T>(lhs.x + rhs),
      static_cast<T>(lhs.y + rhs),
    };
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator+=(Vector<T>& lhs, S rhs) {
    lhs.x += rhs;
    lhs.y += rhs;
    return lhs;
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator-(const Vector<T>& lhs, S rhs) {
    return {
      static_cast<T>(lhs.x - rhs),
      static_cast<T>(lhs.y - rhs),
    };
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator-=(Vector<T>& lhs, S rhs) {
    lhs.x -= rhs;
    lhs.y -= rhs;
    return lhs;
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator*(const Vector<T>& lhs, S rhs) {
    return {
      static_cast<T>(lhs.x * rhs),
      static_cast<T>(lhs.y * rhs),
    };
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator*=(Vector<T>& lhs, S rhs) {
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator/(const Vector<T>& lhs, S rhs) {
    return {
      static_cast<T>(lhs.x / rhs),
      static_cast<T>(lhs.y / rhs),
    };
  }

  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator/=(Vector<T>& lhs, S rhs) {
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
  }

  template <typename T> class LineSegment;

  template <typename T>
  class Line {
  public:

    Line(T a, T b, T c) : a(a), b(b), c(c) {
      if (a == 0 && b == 0) {
        throw std::runtime_error(
          "Line: cannot create line where a and b are both 0"
        );
      }
      normalize();
    }

    Line(const Vector<T>& p1, const Vector<T>& p2)
      : a((p2 - p1).slope()) {
      if (p1 == p2) {
        throw std::runtime_error(
          "Line: cannot create line where a and b are both 0"
        );
      }
      if (a == std::numeric_limits<T>::infinity()) {
        a = 1;
        b = 0;
        c = p1.x;
      } else {
        b = -1;
        c = a * p1.x - p1.y;
      }
      normalize();
    }

    Line(const Vector<T>& p, T slope) {
      if (slope == std::numeric_limits<T>::infinity()) {
        a = 1;
        b = 0;
        c = p.x;
      } else {
        a = slope;
        b = -1;
        c = slope * p.x - p.y;
      }
      normalize();
    }

    Line<T>& normalize() {
      if (a != 0 && b == 0) {
        c = c / a;
        a = 1;
      } else if (a == 0 & b != 0) {
        c = c / b;
        b = 1;
      }
      if (a == 0 && b < 0 && c < 0) {
        b = -b;
        c = -c;
      } else if (a < 0 && b == 0 && c < 0) {
        a = -a;
        c = -c;
      }
      return *this;
    }

    T slope() const {
      if (b == 0) {
        return std::numeric_limits<float>::infinity();
      }
      return -a / b;
    }

    Line<T> normal() const {
      if (a == 0) {
        return Line<T>(1, 0, 0);
      } else if (b == 0) {
        return Line<T>(0, 1, 0);
      }
      return Line<T>(-1 / a, b, c);
    }

    bool contains(
      const Vector<T>& p,
      T epsilon
    ) const {
      return std::abs(a * p.x + b * p.y - c) < epsilon;
    }

    T x_from_y(T y) const {
      return (-b * y + c) / a;
    }

    T y_from_x(T x) const {
      return (-a * x + c) / b;
    }

    Vector<T> unit() const {
      if (b == 0) {
        return {0, 1};
      }
      return Vector<T>::from_slope(slope(), 1);
    }

    Vector<T> x_intercept() const {
      return {c / a, 0};
    }

    Vector<T> y_intercept() const {
      if (b == 0) {
        return Vector<T>::NaN();
      }
      return {0, -c};
    }

    Vector<T> intersection(
      const LineSegment<T>& on,
      T epsilon
    ) const;

    Vector<T> intersection(
      const Line<T>& on
    ) {
      auto d = std::abs(Vector<T>(on.a, on.b).cross(Vector<T>(a, b)));
      if (d) {
        return Vector<T>(
          (b * on.c - c * on.b) / d,
          (c * on.a - a * on.c) / d
        );
      }
      return Vector<T>::NaN();
    }

    std::string to_string() const {
      std::string str;
      if (a) {
        if (a < 0) {
          str += "-";
        }
        if (a != 1 && a != -1) {
          str += std::to_string(a);
        }
        str += "x";
        if (b) {
          if (b < 0) {
            str += " - ";
          } else {
            str += " + ";
          }
          if (b != 1 && b != -1) {
            str += std::to_string(b);
          }
          str += "y";
        }
      } else {
        if (b < 0) {
          str += "-";
        }
        if (b != 1 && b != -1) {
          str += std::to_string(b);
        }
        str += "y";
      }
      str += " = " + std::to_string(c);
      return str;
    }

    T a, b, c;
  };

  template <typename T>
  class LineSegment {
  public:

    class ConstIterator {

      ConstIterator(const LineSegment<T>& line, size_t index, int dir)
        : line(line), index(index), dir(dir) {}
      const LineSegment<T>& line;
      size_t index;
      int dir;

    public:

      const Vector<T>& operator*() const {
        if (index == 0) {
          return line.p;
        } else if (index == 1) {
          return line.q;
        }
        throw std::range_error("Index out of range");
      }

      const Vector<T>* operator->() const {
        if (index == 0) {
          return &line.p;
        } else if (index == 1) {
          return &line.q;
        }
        throw std::range_error("Index out of range");
      }

      ConstIterator& operator++() {
        index += dir;
        return *this;
      }

      ConstIterator& operator--() {
        index -= dir;
        return *this;
      }

      bool operator==(const ConstIterator& rhs) const {
        return &line == &rhs.line && index == rhs.index;
      }

      bool operator!=(const ConstIterator& rhs) const {
        return !(*this == rhs);
      }

      friend class LineSegment;

    };

    class Iterator {

      Iterator(LineSegment<T>& line, size_t index, int dir)
        : line(line), index(index), dir(dir) {}

      LineSegment<T>& line;

      size_t index;

      int dir;

    public:

      Vector<T>& operator*() {
        if (index == 0) {
          return line.p;
        } else if (index == 1) {
          return line.q;
        }
        throw std::range_error("Index out of range");
      }

      Vector<T>* operator->() {
        if (index == 0) {
          return &line.p;
        } else if (index == 1) {
          return &line.q;
        }
        throw std::range_error("Index out of range");
      }

      Iterator operator++(int _) {
        auto copy = *this;
        ++(*this);
        return copy;
      }

      Iterator operator--(int _) {
        auto copy = *this;
        --(*this);
        return copy;
      }

      Iterator& operator++() {
        if (index == 2) {
          index = 0;
        } else {
          index += dir;
        }
        return *this;
      }

      Iterator& operator--() {
        if (index == 0) {
          index = 2;
        } else {
          index -= dir;
        }
        return *this;
      }

      bool operator==(const Iterator& rhs) const {
        return &line == &rhs.line && index == rhs.index;
      }

      bool operator!=(const Iterator& rhs) const {
        return !(*this == rhs);
      }

      operator ConstIterator() const {
        return ConstIterator(line, index, dir);
      }

      friend class LineSegment;

    };

    LineSegment() {}

    LineSegment(const Vector<T>& p, const Vector<T>& q)
      : p(p), q(q) {}

    template <typename S>
    LineSegment<S> as() const {
      return {static_cast<Vector<S>>(p), static_cast<Vector<S>>(q)};
    }

    template <typename S>
    operator LineSegment<S>() const {
      return as<S>();
    }

    bool in_bounds(const Vector<T>& v, T epsilon) const {
      auto x1 = std::min(p.x, q.x);
      auto y1 = std::min(p.y, q.y);
      auto x2 = std::max(p.x, q.x);
      auto y2 = std::max(p.y, q.y);
      return (v.x - x1) >= -epsilon
        && (v.x - x2) <= epsilon
        && (v.y - y1) >= -epsilon
        && (v.y - y2) <= epsilon;
    }

    T slope() const {
      return to_line().slope();
    }

    bool contains(const Vector<T>& v, T epsilon) const {
      if (p == q) {
        return (p - v).length() < epsilon;
      }
      return to_line().contains(v, epsilon) && in_bounds(v, epsilon);
    }

    Vector<T> intersection(
      const LineSegment<T>& on,
      T epsilon
    ) const {
      // Check for the colinear case first.
      if (on.contains(p, epsilon)) {
        return p;
      } else if (on.contains(q, epsilon)) {
        return q;
      } else if (contains(on.p, epsilon)) {
        return on.p;
      } else if (contains(on.q, epsilon)) {
        return on.q;
      }
      // Use Cramer's rule to determine an intersection point.
      auto s1 = q - p;
      auto s2 = on.q - on.p;
      auto d = s1.cross(s2);
      if (std::abs(d) < std::numeric_limits<float>::epsilon()) {
        // Lines are parallel.
        return Vector<T>::NaN();
      }
      auto s = s1.cross(p - on.p) / d;
      auto t = s2.cross(p - on.p) / d;
      if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
        return p + t * s1;
      }
      // Segments do not intersect.
      return Vector<T>::NaN();
    }

    Iterator begin() {
      return Iterator(*this, 0, 1);
    }

    Iterator end() {
      return Iterator(*this, 2, 1);
    }

    ConstIterator begin() const {
      return ConstIterator(*this, 0, 1);
    }

    ConstIterator end() const {
      return ConstIterator(*this, 2, 1);
    }

    Iterator rbegin() {
      return Iterator(*this, 1, -1);
    }

    Iterator rend() {
      return Iterator(*this, -1, -1);
    }

    ConstIterator rbegin() const {
      return ConstIterator(*this, 1, -1);
    }

    ConstIterator rend() const {
      return ConstIterator(*this, -1, -1);
    }

    ConstIterator cbegin() const {
      return begin();
    }

    ConstIterator cend() const {
      return end();
    }

    ConstIterator crbegin() const {
      return rbegin();
    }

    ConstIterator crend() const {
      return rend();
    }

    LineSegment<T> operator+(const Vector<T>& rhs) const {
      return {p + rhs, q + rhs};
    }

    LineSegment<T> operator-(const Vector<T>& rhs) const {
      return {p - rhs, q - rhs};
    }

    Line<T> to_line() const {
      return {p, q};
    }

    Vector<T> to_vector() const {
      return q - p;
    }

    Line<T> normal() const {
      return to_line().normal();
    }

    operator Line<T>() const {
      return to_line();
    }

    std::string to_string() const {
      return p.to_string() + "->" + q.to_string();
    }

    bool operator==(const LineSegment<T>& rhs) const {
      return p == rhs.p && q == rhs.q;
    }

    Vector<T> p, q;
  };

  template <typename T>
  inline Vector<T> Line<T>::intersection(
    const LineSegment<T>& on,
    T epsilon
  ) const {
    // Check for the colinear case first.
    if (contains(on.p, epsilon)) {
      return on.p;
    } else if (contains(on.q, epsilon)) {
      return on.q;
    }
    // Use dot product of line normal to determine intersection point.
    Vector<T> q;
    if (slope() == std::numeric_limits<T>::infinity()) {
      q = x_intercept() - on.p;
    } else {
      q = y_intercept() - on.p;
    }
    auto r = on.q - on.p;
    auto n = normal().unit();
    auto t = q.dot(n) / r.dot(n);
    if (t >= 0 && t <= 1) {
      return on.p + t * r;
    }
    // Segments do not intersect.
    return Vector<T>::NaN();
  }

  template <typename T>
  class Rectangle {
  public:

    Rectangle(Vector<T> position, Vector<T> size)
      : position(position), size(size) {}

    bool contains(Vector<T> pos) {
      return pos.x >= position.x && pos.x <= position.x + size.x
        && pos.y >= position.y && pos.y <= position.y + size.y;
    }

    Vector<T> position;

    Vector<T> size;
  };

}
