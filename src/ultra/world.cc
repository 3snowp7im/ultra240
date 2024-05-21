#include <fstream>
#include <unordered_map>
#include <memory>
#include <ultra240/world.h>
#include "ultra/ultra.h"

namespace ultra {

  const static float epsilon = 1.f / 256;

  World::World(const std::string& name) {
    auto path = ultra::path_manager::data_dir + "/world/" + name + ".bin";
    std::ifstream stream(path);
    // Read number of maps.
    uint16_t map_count = util::read<uint16_t>(stream);
    // Read map header offsets.
    uint32_t map_offsets[map_count];
    for (int i = 0; i < map_count; i++) {
      map_offsets[i] = util::read<uint32_t>(stream);
    }
    // Read number of boundaries.
    uint16_t boundaries_count = util::read<uint16_t>(stream);
    // Read boundary offsets.
    uint32_t boundary_offsets[boundaries_count];
    for (int i = 0; i < boundaries_count; i++) {
      boundary_offsets[i] = util::read<uint32_t>(stream);
    }
    // Read maps.
    maps.reserve(map_count);
    for (int i = 0; i < map_count; i++) {
      stream.seekg(map_offsets[i], stream.beg);
      maps.emplace_back(stream);
    }
    // Read points in boundaries.
    std::vector<std::vector<geometry::Vector<int32_t>>> points(
      boundaries_count
    );
    std::vector<uint8_t> boundary_flags(boundaries_count);
    size_t lines_count = 0;
    for (int i = 0; i < boundaries_count; i++) {
      stream.seekg(boundary_offsets[i], stream.beg);
      boundary_flags[i] = util::read<uint8_t>(stream);
      uint16_t points_count = util::read<uint16_t>(stream);
      points[i].resize(points_count);
      lines_count += points_count;
      for (int j = 0; j < points_count; j++) {
        points[i][j].x = util::read<int32_t>(stream);
        points[i][j].y = util::read<int32_t>(stream);
      }
    }
    // Create line segments from points lists.
    boundaries.reset(new Boundaries(VectorAllocator<Boundary>(lines_count)));
    for (uint i = 0; i < boundaries_count; i++) {
      auto& boundary = points[i];
      uint8_t flags = boundary_flags[i];
      auto a = boundary.cbegin();
      auto b = std::next(a);
      while (b != boundary.cend()) {
        boundaries->emplace_back(flags, *a, *b);
        a++;
        b++;
      }
    }
  }

  static bool can_skip_corner_boundary(
    size_t i,
    const World::Boundary& boundary
  ) {
    if (i == 0) {
      return boundary.p.x < boundary.q.x || boundary.p.y > boundary.q.y;
    } else if (i == 1) {
      return boundary.p.x < boundary.q.x || boundary.p.y < boundary.q.y;
    } else if (i == 2) {
      return boundary.p.x > boundary.q.x || boundary.p.y < boundary.q.y;
    }
    return boundary.p.x > boundary.q.x || boundary.p.y > boundary.q.y;
  }

  static bool can_skip_edge_boundary(
    size_t i,
    const geometry::LineSegment<float>& edge,
    const World::Boundary& boundary
  ) {
    if (i == 0) {
      return boundary.p.x < boundary.q.x || boundary.p.y == boundary.q.y;
    } else if (i == 1) {
      return boundary.p.x == boundary.q.x || boundary.p.y > boundary.q.y;
    } else if (i == 2) {
      return boundary.p.x > boundary.q.x || boundary.p.y == boundary.q.y;
    }
    return boundary.p.x == boundary.q.x || boundary.p.y < boundary.q.y;
  }

  static bool is_stuck(
    geometry::Vector<float> adjust,
    bool moved[4]
  ) {
    // The direction of adjustments are recordered and if there are ever
    // adjustments in opposing cardinal directions, the entity is assumed
    // to be stuck.
    if (adjust.x < 0) {
      moved[0] = true;
    } else if (adjust.x > 0) {
      moved[2] = true;
    }
    if (adjust.y < 0) {
      moved[1] = true;
    } else if (adjust.y > 0) {
      moved[3] = true;
    }
    if ((moved[0] && moved[2]) || (moved[1] && moved[3])) {
      return true;
    }
    return false;
  }

  std::pair<bool, geometry::Vector<float>> World::can_fit_collision_boxes(
    const Tileset::Tile::CollisionBox<float>* prev_collision_boxes,
    size_t prev_collision_boxes_count,
    const Tileset::Tile::CollisionBox<float>* next_collision_boxes,
    size_t next_collision_boxes_count,
    const Boundaries& boundaries,
    bool check_transits
  ) {
    bool moved[4] = {false, false, false, false};
    geometry::Vector<float> offset;
  adjust_position:
    for (size_t i = 0; i < next_collision_boxes_count; i++) {
      const auto& box = next_collision_boxes[i];
      auto pos = box.position + offset;
      geometry::Vector<float> corners[] = {
        pos,
        pos + box.size.as_x(),
        pos + box.size,
        pos + box.size.as_y(),
      };
      geometry::LineSegment<float> transits[prev_collision_boxes_count][4];
      bool is_line[prev_collision_boxes_count][4];
      if (check_transits) {
        geometry::LineSegment<float>* t = transits[0];
        bool* l = is_line[0];
        for (size_t j = 0; j < prev_collision_boxes_count; j++) {
          const auto& box = prev_collision_boxes[j];
          auto pos = box.position + offset;
          if (std::abs((pos - corners[0]).length()) < epsilon) {
            *l++ = false;
            t++;
          } else {
            *l++ = true;
            *t++ = {pos, corners[0]};
          }
          if (std::abs((pos + box.size.as_x() - corners[1]).length())
              < epsilon) {
            *l++ = false;
            t++;
          } else {
            *l++ = true;
            *t++ = {pos + box.size.as_x(), corners[1]};
          }
          if (std::abs((pos + box.size - corners[2]).length())
              < epsilon) {
            *l++ = false;
            t++;
          } else {
            *l++ = true;
            *t++ = {pos + box.size, corners[2]};
          }
          if (std::abs((pos + box.size.as_y() - corners[3]).length())
              < epsilon) {
            *l++ = false;
            t++;
          } else {
            *l++ = true;
            *t++ = {pos + box.size.as_y(), corners[3]};
          }
        }
      }
      for (const auto& boundary : boundaries) {
        const auto& p = boundary.p;
        const auto& q = boundary.q;
        if (check_transits && prev_collision_boxes_count) {
          for (size_t j = 0; j < prev_collision_boxes_count; j++) {
            for (int k = 0; k < 4; k++) {
              if (!can_skip_corner_boundary(k, boundary) && is_line[j][k]) {
                const auto& t = transits[j][k];
                if (t.slope() == boundary.slope()) {
                  continue;
                }
                auto intersection = boundary.intersection(t, epsilon);
                if (!intersection.is_nan()) {
                  auto s = intersection - corners[k];
                  if (is_stuck(s, moved)) {
                    return std::make_pair(false, geometry::Vector<float>{0, 0});
                  }
                  if (s.x || s.y) {
                    offset += s;
                    goto adjust_position;
                  }
                }
              }
            }
          }
        } else {
          geometry::LineSegment<float> edges[] = {
            {corners[0], corners[1]},
            {corners[1], corners[2]},
            {corners[2], corners[3]},
            {corners[3], corners[0]},
          };
          for (int j = 0; j < 4; j++) {
            const auto& edge = edges[j];
            if (!can_skip_edge_boundary(j, edge, boundary)) {
              // Get intersection between new edge and boundary.
              auto intersection = boundary.intersection(edge, epsilon);
              if (!intersection.is_nan()) {
                if (intersection == edge.p
                    || intersection == edge.q
                    || intersection == boundary.p
                    || intersection == boundary.q) {
                  continue;
                }
                if (boundary.flags & World::Boundary::Flags::OneWay) {
                  // Check to see if the current tile collision boxes
                  // intersect with this boundary.
                  bool is_inside = true;
                  bool skip = false;
                  for (size_t k = 0; k < next_collision_boxes_count; k++) {
                    const auto& box = next_collision_boxes[k];
                    auto pos = box.position;
                    if (box.contains(p) || box.contains(q)) {
                      skip = true;
                      goto boxes_loop_break;
                    }
                    geometry::LineSegment<float> edges[] = {
                      {pos, pos + box.size.as_x()},
                      {pos + box.size.as_x(), pos + box.size},
                      {pos + box.size, pos + box.size.as_y()},
                      {pos + box.size.as_y(), pos},
                    };
                    for (const auto& edge : edges) {
                      auto inter = boundary.intersection(edge, epsilon);
                      if (!inter.is_nan()
                          && inter != edge.p
                          && inter != edge.q
                          && inter != boundary.p
                          && inter != boundary.q) {
                        skip = true;
                        goto boxes_loop_break;
                      }
                    }
                  }
                  // Check to see if the current tile collision boxes are
                  // "outside" the boundary.
                  for (size_t k = 0; k < next_collision_boxes_count; k++) {
                    const auto& box = next_collision_boxes[k];
                    auto pos = box.position;
                    geometry::Vector<float> corners[] = {
                      pos,
                      pos + box.size.as_x(),
                      pos + box.size,
                      pos + box.size.as_y(),
                    };
                    for (int l = 0; l < 4; l++) {
                      auto corner = corners[l] - boundary.p;
                      auto bvec = boundary.to_vector();
                      auto cross = bvec.cross(corner);
                      if (bvec.cross(corner) > 0) {
                        is_inside = false;
                        break;
                      }
                    }
                    if (!is_inside) {
                      break;
                    }
                  }
                boxes_loop_break:
                  if (skip || !is_inside) {
                    continue;
                  }
                }
                // Get closest corner to edge.
                geometry::Vector<float> s;
                geometry::Vector<float> corner;
                switch (i) {
                case 0:
                  if (p.y < q.y) {
                    corner = edge.p;
                  } else {
                    corner = edge.q;
                  }
                  break;
                case 1:
                  if (p.x > q.x) {
                    corner = edge.p;
                  } else {
                    corner = edge.q;
                  }
                  break;
                case 2:
                  if (p.y > q.y) {
                    corner = edge.p;
                  } else {
                    corner = edge.q;
                  }
                  break;
                case 3:
                  if (p.x < q.x) {
                    corner = edge.p;
                  } else {
                    corner = edge.q;
                  }
                  break;
                }
                // Define line of expansion for closest corner.
                geometry::Line<float> line(
                  corner,
                  boundary.normal().slope()
                );
                // Get intersection between line of expansion and boundary.
                auto exp_intersection = line.intersection(boundary, epsilon);
                if (exp_intersection.is_nan()) {
                  // If there's no intersection, adjust position in one
                  // dimension so that the box rests on a boundary
                  // endpoint.
                  switch (i) {
                  case 0:
                    if (p.x > q.x) {
                      if (box.contains(p)) {
                        s = {0, corner.y - p.y};
                      } else {
                        s = {corner.x - p.x, 0};
                      }
                    } else {
                      if (box.contains(q)) {
                        s = {0, corner.y - q.y};
                      } else {
                        s = {q.x - corner.x, 0};
                      }
                    }
                    break;
                  case 1:
                    if (p.y < q.y) {
                      if (box.contains(p)) {
                        s = {p.x - corner.x, 0};
                      }  else {
                        s = {0, p.y - corner.y};
                      }
                    } else {
                      if (box.contains(q)) {
                        s = {q.x - corner.x, 0};
                      } else {
                        s = {0, q.y - corner.y};
                      }
                    }
                    break;
                  case 2:
                    if (p.x < q.x) {
                      if (box.contains(p)) {
                        s = {0, p.y - corner.y};
                      } else {
                        s = {p.x - corner.x, 0};
                      }
                    } else {
                      if (box.contains(q)) {
                        s = {0, q.y - corner.y};
                      } else {
                        s = {corner.x - q.x, 0};
                      }
                    }
                    break;
                  case 3:
                    if (p.y > q.y) {
                      if (box.contains(p)) {
                        s = {corner.x - p.x, 0};
                      } else {
                        s = {0, corner.y - p.y};
                      }
                    } else {
                      if (box.contains(q)) {
                        s = {corner.x - q.x, 0};
                      } else {
                        s = {0, corner.y - q.y};
                      }
                    }
                    break;
                  }
                } else {
                  // If there is an intersection, the adjustment is the
                  // delta between the intersection point and the closest
                  // corner.
                  s = exp_intersection - corner;
                }
                if (is_stuck(s, moved)) {
                  return std::make_pair(false, geometry::Vector<float>{0, 0});
                }
                if (s.x || s.y) {
                  offset += s;
                  goto adjust_position;
                }
              }
            }
          }
        }
      }
    }
    return std::make_pair(true, offset);
  }

  std::pair<bool, World::BoundaryCollision> World::get_boundary_collision(
    geometry::Vector<float> force,
    const Tileset::Tile::CollisionBox<float>* collision_boxes,
    size_t collision_boxes_count,
    const World::Boundaries& boundaries
  ) {
    BoundaryCollision closest = {{.distance = geometry::Vector<float>::NaN()}};
    if (force.x != 0 || force.y != 0) {
      for (size_t i = 0; i < collision_boxes_count; i++) {
        const auto& box = collision_boxes[i];
        auto pos = box.position;
        // Check for intersections between boundaries and the transits of the
        // bounding box corners to their new positions.
        typename World::Boundaries::const_iterator curr;
        if (force.x > 0) {
          curr = boundaries.cbegin();
        } else {
          curr = std::prev(boundaries.cend());
        }
        while (curr != boundaries.cend()) {
          const auto& p = curr->p;
          const auto& q = curr->q;
          geometry::Vector<float> corners[4] = {
            pos,
            pos + box.size.as_x(),
            pos + box.size,
            pos + box.size.as_y(),
          };
          geometry::LineSegment<float> transit(pos, pos + force);
          geometry::LineSegment<float> segments[4] = {
            transit,
            transit + box.size.as_x(),
            transit + box.size,
            transit + box.size.as_y(),
          };
          for (int j = 0; j < 4; j++) {
            const auto& segment = segments[j];
            const auto& corner = corners[j];
            switch (j) {
            case 0:
              if ((p.x <= q.x || p.y != q.y)
                  && (p.x != q.x || p.y >= q.y)
                  && (p.x <= q.x || p.y >= q.y)) {
                continue;
              }
              if (curr->slope() == std::numeric_limits<float>::infinity()
                  && segment.contains(q, epsilon)) {
                continue;
              }
              break;
            case 1:
              if ((p.x <= q.x || p.y != q.y)
                  && (p.x != q.x || p.y <= q.y)
                  && (p.x <= q.x || p.y <= q.y)) {
                continue;
              }
              if (curr->slope() == std::numeric_limits<float>::infinity()
                  && segment.contains(p, epsilon)) {
                continue;
              }
              break;
            case 2:
              if ((p.x >= q.x || p.y != q.y)
                  && (p.x != q.x || p.y <= q.y)
                  && (p.x >= q.x || p.y <= q.y)) {
                continue;
              }
              if (curr->slope() == std::numeric_limits<float>::infinity()
                  && segment.contains(q, epsilon)) {
                continue;
              }
              break;
            case 3:
              if ((p.x >= q.x || p.y != q.y)
                  && (p.x != q.x || p.y >= q.y)
                  && (p.x >= q.x || p.y >= q.y)) {
                continue;
              }
              if (curr->slope() == std::numeric_limits<float>::infinity()
                  && segment.contains(p, epsilon)) {
                continue;
              }
              break;
            }
            auto intersection = curr->intersection(segment, epsilon);
            if (!intersection.is_nan()) {
              // Ignore tangential forces intersecting at a boundary edge.
              // Otherwise, entities get caught on top of walls when jumping.
              if (intersection == curr->p || intersection == curr->q) {
                auto cross = curr->to_vector().unit().cross(
                  segment.to_vector().unit()
                );
                if (std::abs(cross) == 1) {
                  continue;
                }
              }
              auto dst = intersection - corner;
              if (closest.distance.is_nan()
                  || dst.length() < closest.distance.length()) {
                switch (j) {
                case 0:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Collision::Edge::Left;
                  } else {
                    closest.edge = Collision::Edge::Top;
                  }
                  break;
                case 1:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Collision::Edge::Right;
                  } else {
                    closest.edge = Collision::Edge::Top;
                  }
                  break;
                case 2:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Collision::Edge::Right;
                  } else {
                    closest.edge = Collision::Edge::Bottom;
                  }
                  break;
                case 3:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Collision::Edge::Left;
                  } else {
                    closest.edge = Collision::Edge::Bottom;
                  }
                  break;
                }
                closest.name = box.name;
                closest.distance = dst;
                closest.boundary = curr;
              }
            }
          }
          if (force.x > 0) {
            curr++;
          } else {
            curr--;
          }
        }
        // Check for boundaries within the transits of the edges to their new
        // positions.
        if (force.x > 0) {
          curr = boundaries.cbegin();
        } else {
          curr = std::prev(boundaries.cend());
        }
        while (curr != boundaries.cend()) {
          const auto& p = curr->p;
          const auto& q = curr->q;
          if (force.y == 0) {
            if (force.x < 0) {
              if (p.x >= pos.x + force.x
                  && p.x <= pos.x
                  && p.y >= pos.y
                  && p.y <= pos.y + box.size.y
                  && q.x >= pos.x + force.x
                  && q.x <= pos.x
                  && q.y >= pos.y
                  && q.y <= pos.y + box.size.y) {
                auto pdst = (pos - p).as_x();
                auto qdst = (pos - q).as_x();
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Left;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            } else if (force.x > 0) {
              if (p.x >= pos.x + box.size.x
                  && p.x <= pos.x + box.size.x + force.x
                  && p.y >= pos.y
                  && p.y <= pos.y + box.size.y
                  && q.x >= pos.x + box.size.x
                  && q.x <= pos.x + box.size.x + force.x
                  && q.y >= pos.y
                  && q.y <= pos.y + box.size.y) {
                auto pdst = (p - pos - box.size).as_x();
                auto qdst = (q - pos - box.size).as_x();
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Right;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            }
          } else if (force.x == 0) {
            if (force.y < 0) {
              if (p.y >= pos.y + force.y
                  && p.y <= pos.y
                  && p.x >= pos.x
                  && p.x <= pos.x + box.size.x
                  && q.y >= pos.y + force.y
                  && q.y <= pos.y
                  && q.x >= pos.x
                  && q.x <= pos.x + box.size.x) {
                auto pdst = (pos - p).as_y();
                auto qdst = (pos - q).as_y();
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Top;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            } else if (force.y > 0) {
              if (p.y >= pos.y + box.size.y
                  && p.y <= pos.y + box.size.y + force.y
                  && p.x >= pos.x
                  && p.x <= pos.x + box.size.x
                  && q.y >= pos.y + box.size.y
                  && q.y <= pos.y + box.size.y + force.y
                  && q.x >= pos.x
                  && q.x <= pos.x + box.size.x) {
                auto pdst = (p - pos - box.size).as_y();
                auto qdst = (q - pos - box.size).as_y();
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Bottom;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            }
          } else if (force.y < 0) {
            geometry::LineSegment<float> left(pos, pos + force);
            auto right = left + box.size.as_x();
            if (p.y >= pos.y + force.y
                && p.y <= pos.y
                && p.x >= left.to_line().x_from_y(p.y)
                && p.x <= right.to_line().x_from_y(p.y)
                && q.y >= pos.y + force.y
                && q.y <= pos.y
                && q.x >= left.to_line().x_from_y(q.y)
                && q.x <= right.to_line().x_from_y(q.y)) {
              auto pdst = geometry::Vector<float>(
                left.to_line().x_from_y(p.y),
                p.y
              ) - left.p;
              auto qdst = geometry::Vector<float>(
                left.to_line().x_from_y(q.y),
                q.y
              ) - left.q;
              auto dst = pdst.length() < qdst.length() ? pdst : qdst;
              if (closest.distance.is_nan()
                  || dst.length() < closest.distance.length()) {
                closest.edge = Collision::Edge::Top;
                closest.name = box.name;
                closest.distance = dst;
                closest.boundary = curr;
              }
            }
            if (force.x < 0) {
              auto bottom = left + box.size.as_y();
              if (p.x >= pos.x + force.x
                  && p.x <= pos.x
                  && p.y >= left.to_line().y_from_x(p.x)
                  && p.y <= bottom.to_line().y_from_x(p.x)
                  && q.x >= pos.x + force.x
                  && q.x <= pos.x
                  && q.y >= left.to_line().y_from_x(q.x)
                  && q.y <= bottom.to_line().y_from_x(q.x)) {
                auto pdst = geometry::Vector<float>(
                  p.x,
                  left.to_line().y_from_x(p.x)
                ) - left.p;
                auto qdst = geometry::Vector<float>(
                  q.x,
                  left.to_line().y_from_x(q.x)
                ) - left.q;
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Left;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            } else {
              auto bottom = right + box.size.as_y();
              if (p.x >= pos.x + box.size.x
                  && p.x <= pos.x + box.size.x + force.x
                  && p.y >= right.to_line().y_from_x(p.x)
                  && p.y <= bottom.to_line().y_from_x(p.x)
                  && q.x >= pos.x + box.size.x
                  && q.x <= pos.x + box.size.x + force.x
                  && q.y >= right.to_line().y_from_x(q.x)
                  && q.y <= bottom.to_line().y_from_x(q.x)) {
                auto pdst = geometry::Vector<float>(
                  p.x,
                  right.to_line().y_from_x(p.x)
                ) - right.p;
                auto qdst = geometry::Vector<float>(
                  q.x,
                  right.to_line().y_from_x(q.x)
                ) - right.q;
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Right;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            }
          } else {
            geometry::LineSegment<float> left(
              pos + box.size.as_y(),
              pos + box.size.as_y() + force
            );
            auto right = left + box.size.as_x();
            if (p.y >= pos.y + box.size.y
                && p.y <= pos.y + box.size.y + force.y
                && p.x >= left.to_line().x_from_y(p.y)
                && p.x <= right.to_line().x_from_y(p.y)
                && q.y >= pos.y + box.size.y
                && q.y <= pos.y + box.size.y + force.y
                && q.x >= left.to_line().x_from_y(q.y)
                && q.x <= right.to_line().x_from_y(q.y)) {
              auto pdst = geometry::Vector<float>(
                left.to_line().x_from_y(p.y),
                p.y
              ) - left.p;
              auto qdst = geometry::Vector<float>(
                left.to_line().x_from_y(q.y),
                q.y
              ) - left.q;
              auto dst = pdst.length() < qdst.length() ? pdst : qdst;
              if (closest.distance.is_nan()
                  || dst.length() < closest.distance.length()) {
                closest.edge = Collision::Edge::Bottom;
                closest.name = box.name;
                closest.distance = dst;
                closest.boundary = curr;
              }
            }
            if (force.x < 0) {
              auto top = left - box.size.as_y();
              if (p.x >= pos.x + force.x
                  && p.x <= pos.x
                  && p.y <= left.to_line().y_from_x(p.x)
                  && p.y >= top.to_line().y_from_x(p.x)
                  && q.x >= pos.x + force.x
                  && q.x <= pos.x
                  && q.y <= left.to_line().y_from_x(q.x)
                  && q.y >= top.to_line().y_from_x(q.x)) {
                auto pdst = geometry::Vector<float>(
                  p.x,
                  left.to_line().y_from_x(p.x)
                ) - left.p;
                auto qdst = geometry::Vector<float>(
                  q.x,
                  left.to_line().y_from_x(q.x)
                ) - left.q;
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Left;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            } else {
              auto top = right - box.size.as_y();
              if (p.x >= pos.x + box.size.x
                  && p.x <= pos.x + box.size.x + force.x
                  && p.y <= right.to_line().y_from_x(p.x)
                  && p.y >= top.to_line().y_from_x(p.x)
                  && q.x >= pos.x + box.size.x
                  && q.x <= pos.x + box.size.x + force.x
                  && q.y <= right.to_line().y_from_x(q.x)
                  && q.y >= top.to_line().y_from_x(q.x)) {
                auto pdst = geometry::Vector<float>(
                  p.x,
                  right.to_line().y_from_x(p.x)
                ) - right.p;
                auto qdst = geometry::Vector<float>(
                  q.x,
                  right.to_line().y_from_x(q.x)
                ) - right.q;
                auto dst = pdst.length() < qdst.length() ? pdst : qdst;
                if (closest.distance.is_nan()
                    || dst.length() < closest.distance.length()) {
                  closest.edge = Collision::Edge::Right;
                  closest.name = box.name;
                  closest.distance = dst;
                  closest.boundary = curr;
                }
              }
            }
          }
          if (force.x > 0) {
            curr++;
          } else {
            curr--;
          }
        }
      }
    }
    if (closest.distance.is_nan()) {
      return std::make_pair(false, BoundaryCollision{});
    }
    return std::make_pair(true, closest);
  }

  World::Boundary::Boundary()
    : geometry::LineSegment<float>(),
      flags(0) {}

  World::Boundary::Boundary(
    const geometry::Vector<float>& p,
    const geometry::Vector<float>& q
  ) : geometry::LineSegment<float>(p, q),
      flags(0) {}

  World::Boundary::Boundary(
    uint8_t flags,
    const geometry::Vector<float>& p,
    const geometry::Vector<float>& q
  ) : geometry::LineSegment<float>(p, q),
      flags(flags) {}

  const World::Boundaries& World::get_boundaries() const {
    return *boundaries;
  }

  World::Map::Map(std::istream& stream) {
    // Read map position in world.
    position.x = util::read<int16_t>(stream);
    position.y = util::read<int16_t>(stream);
    // Read map width and height.
    size.x = util::read<uint16_t>(stream);
    size.y = util::read<uint16_t>(stream);
    // Read properties.
    uint8_t properties_count = util::read<uint8_t>(stream);
    for (int i = 0; i < properties_count; i++) {
      properties.emplace(
        util::read<Hash>(stream),
        util::read<uint32_t>(stream)
      );
    }
    // Read map tileset count.
    uint8_t map_tileset_count = util::read<uint8_t>(stream);
    // Maintain a set of tileset offsets.
    std::map<uint32_t, std::shared_ptr<Tileset>> tilesets;
    // Read map tileset offsets.
    uint32_t map_tileset_offsets[map_tileset_count];
    for (int i = 0; i < map_tileset_count; i++) {
      uint32_t offset = util::read<uint32_t>(stream);
      tilesets.emplace(offset, nullptr);
      map_tileset_offsets[i] = offset;
    }
    // Read entity tileset count.
    uint8_t entity_tileset_count = util::read<uint8_t>(stream);
    // Read entity tileset offsets.
    uint32_t entity_tileset_offsets[entity_tileset_count];
    for (int i = 0; i < entity_tileset_count; i++) {
      uint32_t offset = util::read<uint32_t>(stream);
      tilesets.emplace(offset, nullptr);
      entity_tileset_offsets[i] = offset;
    }
    // Read layer count.
    uint8_t layer_count = util::read<uint8_t>(stream);
    // Read layer offsets.
    uint32_t layer_offsets[layer_count];
    for (int i = 0; i < layer_count; i++) {
      layer_offsets[i] = util::read<uint32_t>(stream);
    }
    // Read entity count.
    uint16_t entity_count = util::read<uint16_t>(stream);
    // Store entities offset.
    size_t entity_offset = stream.tellg();
    // Read sorted entities index.
    stream.seekg(
      entity_offset
      + entity_count * (sizeof(Hash) + 5 * sizeof(uint16_t) + sizeof(uint32_t))
    );
    sorted_entities.x.min.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      sorted_entities.x.min[i] = util::read<uint16_t>(stream);
    }
    sorted_entities.x.max.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      sorted_entities.x.max[i] = util::read<uint16_t>(stream);
    }
    sorted_entities.y.min.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      sorted_entities.y.min[i] = util::read<uint16_t>(stream);
    }
    sorted_entities.y.max.resize(entity_count);
    for (int i = 0; i < entity_count; i++) {
      sorted_entities.y.max[i] = util::read<uint16_t>(stream);
    }
    // Read map tilesets.
    map_tilesets.resize(map_tileset_count);
    for (int i = 0; i < map_tileset_count; i++) {
      stream.seekg(map_tileset_offsets[i], stream.beg);
      tilesets.at(map_tileset_offsets[i]).reset(new Tileset(stream));
      map_tilesets[i] = tilesets.at(map_tileset_offsets[i]);
    }
    // Read entity tilesets.
    entity_tilesets.resize(entity_tileset_count);
    for (int i = 0; i < entity_tileset_count; i++) {
      stream.seekg(entity_tileset_offsets[i], stream.beg);
      if (tilesets.at(entity_tileset_offsets[i]) == nullptr) {
        tilesets.at(entity_tileset_offsets[i]).reset(new Tileset(stream));
      }
      entity_tilesets[i] = tilesets.at(entity_tileset_offsets[i]);
    }
    // Read layers.
    size_t area = size.x * size.y;
    tiles.resize(layer_count * area);
    layers.reserve(layer_count);
    for (int i = 0; i < layer_count; i++) {
      // Read layer.
      stream.seekg(layer_offsets[i], stream.beg);
      layers.emplace_back(stream);
      // Read tiles.
      util::read<uint16_t>(&tiles[i * area], stream, area);
    }
    // Read entities.
    entities.reserve(entity_count);
    stream.seekg(entity_offset);
    for (int i = 0; i < entity_count; i++) {
      entities.emplace_back(&entity_tilesets[0], stream);
    }
  }

  static geometry::Vector<float> read_parallax(std::istream& stream) {
    float xn = util::read<uint8_t>(stream);
    float xd = util::read<uint8_t>(stream);
    float yn = util::read<uint8_t>(stream);
    float yd = util::read<uint8_t>(stream);
    return {xn / xd, yn / yd};
  }

  World::Map::Layer::Layer(std::istream& stream)
    : name(util::read<Hash>(stream)),
      parallax(read_parallax(stream)) {}

  World::Map::Entity::Entity(
    const std::shared_ptr<Tileset>* entity_tilesets,
    std::istream& stream
  ) : layer_name(util::read<Hash>(stream)),
      position(util::read_vector<uint16_t>(stream)),
      tile_index(util::read<uint16_t>(stream)),
      type(util::read<uint16_t>(stream)),
      id(util::read<uint16_t>(stream)),
      state(util::read<uint32_t>(stream)),
      tileset(*entity_tilesets[tile_index >> 12]),
      attributes({
        .flip_x = !!(tile_index & 0x800),
        .flip_y = !!(tile_index & 0x400),
      }) {
    tile_index = (tile_index & 0x3ff) - 1;
  }

}
