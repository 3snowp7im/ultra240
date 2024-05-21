#pragma once

#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>

namespace ultra::geometry {

  /** A generic vector. */
  template <typename T>
  class Vector {
  public:

    /** Instantiate from a slope and magnitude. */
    static inline Vector<T> from_slope(T slope, T magnitude) {
      if (slope == std::numeric_limits<T>::infinity()
          || slope == -std::numeric_limits<T>::infinity()) {
        return Vector<T>(0, magnitude);
      }
      return Vector<T>(1, slope).unit() * magnitude;
    }

    /** Instantiate a NaN vector. */
    static Vector<T> NaN() {
      return Vector<T>(
        std::numeric_limits<T>::signaling_NaN(),
        std::numeric_limits<T>::signaling_NaN()
      );
    }

    /** Instantiate a zero vector. */
    Vector() : x(0), y(0) {}

    /** Instantiate a vector from x and y components. */
    Vector(T x, T y) : x(x), y(y) {}

    /** Return a new vector with components cast to specified type. */
    template <typename S>
    Vector<S> as() const {
      return {static_cast<S>(x), static_cast<S>(y)};
    }

    /** Cast operator. */
    template <typename S>
    operator Vector<S>() const {
      return as<S>();
    }

    /** Assignment from a scalar. */
    template <typename S>
    Vector<T>& operator=(S rhs) {
      x = rhs;
      y = rhs;
      return *this;
    }

    /** Vector addition operator. */
    Vector<T> operator+(const Vector<T>& rhs) const {
      return {x + rhs.x, y + rhs.y};
    }

    /** Vector add and assign operator. */
    Vector<T>& operator+=(const Vector<T>& rhs) {
      x += rhs.x;
      y += rhs.y;
      return *this;
    }

    /** Vector subtraction operator. */
    Vector<T> operator-(const Vector<T>& rhs) const {
      return {x - rhs.x, y - rhs.y};
    }

    /** Vector subtract and assign operator. */
    Vector<T>& operator-=(const Vector<T>& rhs) {
      x -= rhs.x;
      y -= rhs.y;
      return *this;
    }

    /** Vector multiply operator. */
    Vector<T> operator*(const Vector<T>& rhs) const {
      return {x * rhs.x, y * rhs.y};
    }

    /** Vector multiply and assign operator. */
    Vector<T>& operator*=(const Vector<T>& rhs) {
      x *= rhs.x;
      y *= rhs.y;
      return *this;
    }

    /** Vector divide operator. */
    Vector<T> operator/(const Vector<T>& rhs) const {
      return {x / rhs.x, y / rhs.y};
    }

    /** Vector divide and assign operator. */
    Vector<T>& operator/=(const Vector<T>& rhs) {
      x /= rhs.x;
      y /= rhs.y;
      return *this;
    }

    /** Vector equality test operator. */
    bool operator==(const Vector<T>& rhs) const {
      return x == rhs.x && y == rhs.y;
    }

    /** Vector inequality test operatar. */
    bool operator!=(const Vector<T>& rhs) {
      return !(*this == rhs);
    }

    /** Vector add scalar operator. */
    Vector<T> operator+(T rhs) const {
      return {x + rhs, y + rhs};
    }

    /** Vector subtract scalar operator. */
    Vector<T> operator-(T rhs) const {
      return {x - rhs, y - rhs};
    }

    /** Vector multiply scalar operator. */
    Vector<T> operator*(T rhs) const {
      return {x * rhs, y * rhs};
    }

    /** Vector divide scalar operator. */
    Vector<T> operator/(T rhs) const {
      return {x / rhs, y / rhs};
    }

    /** Return true if vector components are NaN. */
    bool is_nan() const {
      return std::isnan(x) || std::isnan(y);
    }

    /** Return vector with y component zerod out. */
    Vector<T> as_x() const {
      return {x, 0};
    }

    /** Return vector with x component zerod out. */
    Vector<T> as_y() const {
      return {0, y};
    }

    /** Return vector slope. */
    T slope() const {
      if (x == 0) {
        return std::numeric_limits<T>::infinity();
      }
      return y / x;
    }

    /** Return vector length. */
    T length() const {
      return std::sqrt(x * x + y * y);
    }

    /** Return cross product of two vectors. */
    T cross(const Vector<T>& rhs) const {
      return x * rhs.y - y * rhs.x;
    }

    /** Return dot product of two vectors. */
    T dot(const Vector<T>& rhs) const {
      return x * rhs.x + y * rhs.y;
    }

    /** Return a unit vector. */
    Vector<T> unit() const {
      return *this / length();
    }

    /** Return a string representation of the vector. */
    std::string to_string() const {
      return "{" + std::to_string(x) + "," + std::to_string(y) + "}";
    }

    T x, y;
  };

  /** Scalar add vector operator. */
  template <typename T>
  Vector<T> operator+(T lhs, const Vector<T>& rhs) {
    return {
      static_cast<T>(lhs + rhs.x),
      static_cast<T>(lhs + rhs.y),
    };
  }

  /** Scalar subtract vector operator. */
  template <typename T>
  Vector<T> operator-(T lhs, const Vector<T>& rhs) {
    return {
      static_cast<T>(lhs - rhs.x),
      static_cast<T>(lhs - rhs.y),
    };
  }

  /** Scalar multiply vector operator. */
  template <typename T>
  Vector<T> operator*(T lhs, const Vector<T>& rhs) {
    return {
      static_cast<T>(lhs * rhs.x),
      static_cast<T>(lhs * rhs.y),
    };
  }

  /** Scalar divide vector operator. */
  template <typename T>
  Vector<T> operator/(T lhs, const Vector<T>& rhs) {
    return {
      static_cast<T>(lhs / rhs.x),
      static_cast<T>(lhs / rhs.y),
    };
  }

  /** Vector add scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator+(const Vector<T>& lhs, S rhs) {
    return {lhs.x + rhs, lhs.y + rhs};
  }

  /** Vector add and assign scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator+=(Vector<T>& lhs, S rhs) {
    lhs.x += rhs;
    lhs.y += rhs;
    return lhs;
  }

  /** Vector subtract scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator-(const Vector<T>& lhs, S rhs) {
    return {lhs.x - rhs, lhs.y - rhs};
  }

  /** Vector subtract and assign scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator-=(Vector<T>& lhs, S rhs) {
    lhs.x -= rhs;
    lhs.y -= rhs;
    return lhs;
  }

  /** Vector multiply scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator*(const Vector<T>& lhs, S rhs) {
    return {lhs.x * rhs, lhs.y * rhs};
  }

  /** Vector multiply and assign scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator*=(Vector<T>& lhs, S rhs) {
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
  }

  /** Vector divide scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>>::type
  operator/(const Vector<T>& lhs, S rhs) {
    return {lhs.x / rhs, lhs.y / rhs};
  }

  /** Vector divide and assign scalar operator. */
  template <typename T, typename S>
  typename std::enable_if<std::is_scalar<S>::value, Vector<T>&>::type
  operator/=(Vector<T>& lhs, S rhs) {
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
  }

  template <typename T> class LineSegment;

  /** A generic line. */ 
  template <typename T>
  class Line {
  public:

    /** Create line from line equation in the form ax + by = c. */
    Line(T a, T b, T c) : a(a), b(b), c(c) {
      if (a == 0 && b == 0) {
        throw std::runtime_error(
          "Line: cannot create line where a and b are both 0"
        );
      }
      normalize();
    }

    /** Create line from two points. */
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

    /** Create line from point and slope. */
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

    /** Normalize line equation so that either a or b are either 0 or 1. */
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

    /** Return the slope of the line. */
    T slope() const {
      if (b == 0) {
        return std::numeric_limits<float>::infinity();
      }
      return -a / b;
    }

    /** Return a line perpendicular to the line. */
    Line<T> normal() const {
      if (a == 0) {
        return Line<T>(1, 0, 0);
      } else if (b == 0) {
        return Line<T>(0, 1, 0);
      }
      return Line<T>(-1 / a, b, c);
    }

    /** Return true if the line contains a specified point. */
    bool contains(
      const Vector<T>& p,
      T epsilon
    ) const {
      return std::abs(a * p.x + b * p.y - c) < epsilon;
    }

    /** Solve for x in line equation given a specified y value. */
    T x_from_y(T y) const {
      return (-b * y + c) / a;
    }

    /** Solve for y in line equation given a specified x value. */
    T y_from_x(T x) const {
      return (-a * x + c) / b;
    }

    /** Return a unit vector in the same direction as the line. */
    Vector<T> unit() const {
      if (b == 0) {
        return {0, 1};
      }
      return Vector<T>::from_slope(slope(), 1);
    }

    /** Return the line's x intercept. */
    Vector<T> x_intercept() const {
      return {c / a, 0};
    }

    /** Return the line's y intercept. */
    Vector<T> y_intercept() const {
      if (b == 0) {
        return Vector<T>::NaN();
      }
      return {0, -c};
    }

    /** Calculate the intersection of the line and a line segment. */
    Vector<T> intersection(
      const LineSegment<T>& on,
      T epsilon
    ) const;

    /** Calculate the intersection of two lines. */
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

    /** Return a string representation of the line. */
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

  /** A generic line segment. */
  template <typename T>
  class LineSegment {
  public:

    /**
     * A const iterator that will iterate through the vectors comprising the
     * line segment.
     */
    class ConstIterator {

      ConstIterator(const LineSegment<T>& line, size_t index, int dir)
        : line(line), index(index), dir(dir) {}

      const LineSegment<T>& line;

      size_t index;

      int dir;

    public:

      /** Derefence operator. */
      const Vector<T>& operator*() const {
        if (index == 0) {
          return line.p;
        } else if (index == 1) {
          return line.q;
        }
        throw std::range_error("Index out of range");
      }

      /** Arrow operator. */
      const Vector<T>* operator->() const {
        if (index == 0) {
          return &line.p;
        } else if (index == 1) {
          return &line.q;
        }
        throw std::range_error("Index out of range");
      }

      /** Post increment operator. */
      ConstIterator& operator++() {
        index += dir;
        return *this;
      }

      /** Post decrement operator. */
      ConstIterator& operator--() {
        index -= dir;
        return *this;
      }

      /** Equality test operator. */
      bool operator==(const ConstIterator& rhs) const {
        return &line == &rhs.line && index == rhs.index;
      }

      /** Inequality test operator. */
      bool operator!=(const ConstIterator& rhs) const {
        return !(*this == rhs);
      }

      friend class LineSegment;

    };

    /**
     * An iterator that will iterate through the vectors comprising the line
     * segment.
     */
    class Iterator {

      Iterator(LineSegment<T>& line, size_t index, int dir)
        : line(line), index(index), dir(dir) {}

      LineSegment<T>& line;

      size_t index;

      int dir;

    public:

      /** Derefence operator. */
      Vector<T>& operator*() {
        if (index == 0) {
          return line.p;
        } else if (index == 1) {
          return line.q;
        }
        throw std::range_error("Index out of range");
      }

      /** Arrow operator. */
      Vector<T>* operator->() {
        if (index == 0) {
          return &line.p;
        } else if (index == 1) {
          return &line.q;
        }
        throw std::range_error("Index out of range");
      }

      /** Pre increment operator. */
      Iterator operator++(int _) {
        auto copy = *this;
        ++(*this);
        return copy;
      }

      /** Pre decrement operator. */
      Iterator operator--(int _) {
        auto copy = *this;
        --(*this);
        return copy;
      }

      /** Post increment operator. */ 
      Iterator& operator++() {
        if (index == 2) {
          index = 0;
        } else {
          index += dir;
        }
        return *this;
      }

      /** Post decrement operator. */
      Iterator& operator--() {
        if (index == 0) {
          index = 2;
        } else {
          index -= dir;
        }
        return *this;
      }

      /** Equality test operator. */
      bool operator==(const Iterator& rhs) const {
        return &line == &rhs.line && index == rhs.index;
      }

      /** Inequality test operator. */
      bool operator!=(const Iterator& rhs) const {
        return !(*this == rhs);
      }

      /** Cast to const iterator operator. */
      operator ConstIterator() const {
        return ConstIterator(line, index, dir);
      }

      friend class LineSegment;

    };

    /** Instance constructor. */
    LineSegment() {}

    /** Construct instance from two vectors. */
    LineSegment(const Vector<T>& p, const Vector<T>& q)
      : p(p), q(q) {}

    /** Cast vector components as specified type. */
    template <typename S>
    LineSegment<S> as() const {
      return {static_cast<Vector<S>>(p), static_cast<Vector<S>>(q)};
    }

    /** Cast operator. */
    template <typename S>
    operator LineSegment<S>() const {
      return as<S>();
    }

    /**
     * Return true if a vector falls within the rectangle defined by line
     * segment's vectors..
     */
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

    /** Return the slope of the line segment. */
    T slope() const {
      return to_line().slope();
    }

    /** Return true if the line segment contains the specified vector. */
    bool contains(const Vector<T>& v, T epsilon) const {
      if (p == q) {
        return (p - v).length() < epsilon;
      }
      return to_line().contains(v, epsilon) && in_bounds(v, epsilon);
    }

    /** Calculate the intersection of two line segments. */
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

    /** Return an iterator starting at the line segment's first vector. */
    Iterator begin() {
      return Iterator(*this, 0, 1);
    }

    /** Return an iterator starting after the line segment's second vector. */
    Iterator end() {
      return Iterator(*this, 2, 1);
    }

    /** Return a const interator starting at the line segment's first vector. */
    ConstIterator begin() const {
      return ConstIterator(*this, 0, 1);
    }

    /** 
     * Return a const interator starting after at the line segment's second 
     * vector.
     */
    ConstIterator end() const {
      return ConstIterator(*this, 2, 1);
    }

    /**
     * Return a reverse iterator starting at the line segment's first vector.
     */
    Iterator rbegin() {
      return Iterator(*this, 1, -1);
    }

    /**
     * Return a reverse iterator starting after the line segment's second
     * vector.
     */
    Iterator rend() {
      return Iterator(*this, -1, -1);
    }

    /**
     * Return a reverse const iterator starting at the line segment's first
     * vector.
     */
    ConstIterator rbegin() const {
      return ConstIterator(*this, 1, -1);
    }

    /**
     * Return a reverse const iterator starting after the line segment's second
     * vector.
     */
    ConstIterator rend() const {
      return ConstIterator(*this, -1, -1);
    }

    /** Return a const interator starting at the line segment's first vector. */
    ConstIterator cbegin() const {
      return begin();
    }

    /** 
     * Return a const interator starting after at the line segment's second 
     * vector.
     */
    ConstIterator cend() const {
      return end();
    }

    /**
     * Return a reverse const iterator starting at the line segment's first
     * vector.
     */
    ConstIterator crbegin() const {
      return rbegin();
    }

    /**
     * Return a reverse const iterator starting after the line segment's second
     * vector.
     */
    ConstIterator crend() const {
      return rend();
    }

    /** Add a vector to the line segment's vectors. */
    LineSegment<T> operator+(const Vector<T>& rhs) const {
      return {p + rhs, q + rhs};
    }

    /** Subtract a vector from the line segment's vectors. */
    LineSegment<T> operator-(const Vector<T>& rhs) const {
      return {p - rhs, q - rhs};
    }

    /** Return the line segment as a line equation. */
    Line<T> to_line() const {
      return {p, q};
    }

    /** Return the difference between the second and first vectors. */
    Vector<T> to_vector() const {
      return q - p;
    }

    /** Return a line perpendicular to the line segment. */
    Line<T> normal() const {
      return to_line().normal();
    }

    /** Cast a line equation. */
    operator Line<T>() const {
      return to_line();
    }

    /** Equality test operator. */
    bool operator==(const LineSegment<T>& rhs) const {
      return p == rhs.p && q == rhs.q;
    }

    /** Return a string representation of the line segment. */
    std::string to_string() const {
      return p.to_string() + "->" + q.to_string();
    }

    Vector<T> p, q;
  };

  /** Calculate intersection of a line and a line segment. */
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

  /** Generic rectangle class. */
  template <typename T>
  class Rectangle {
  public:

    /** Instance constructor. */
    Rectangle(Vector<T> position, Vector<T> size)
      : position(position), size(size) {}

    /** Return true if rectangle contains the specified point. */
    bool contains(Vector<T> pos) const {
      return pos.x >= position.x && pos.x <= position.x + size.x
        && pos.y >= position.y && pos.y <= position.y + size.y;
    }

    /** Add vector to rectangle position. */
    Rectangle operator+(Vector<T> pos) const {
      return Rectangle(position + pos, size);
    }

    /** Subtract vector from rectange position. */
    Rectangle operator-(Vector<T> pos) const {
      return Rectangle(position - pos, size);
    }

    /** Return a string representation of the rectangle. */
    std::string to_string() const {
      return "{" + position.to_string() + "," + size.to_string() + "}";
    }

    Vector<T> position;

    Vector<T> size;
  };

}
