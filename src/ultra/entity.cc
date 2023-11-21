#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <ultra240/entity.h>

namespace ultra {

  const static float epsilon = 1.f / 256;

  bool Entity::has_collision_boxes(
    Hash<>::Type type
  ) const {
    return !!tileset.tiles[tile_index].collision_boxes.count(type);
  }

  const Tileset::Tile::CollisionBox::NamedList&
  Entity::get_collision_boxes(
    Hash<>::Type type
  ) const {
    return tileset.tiles[tile_index].collision_boxes.at(type);
  }

  geometry::Vector<float> Entity::get_collision_box_position(
    const Tileset::Tile::CollisionBox& box
  ) const {
    return get_collision_box_position(position, box);
  }

  geometry::Vector<float> Entity::get_collision_box_position(
    const geometry::Vector<float>& pos,
    const Tileset::Tile::CollisionBox& box
  ) const {
    if (attributes.flip_x && attributes.flip_y) {
      return pos
        + tileset.tile_size.as_x()
        - box.position
        - box.size;
    } else if (attributes.flip_y) {
      return pos
        + box.position.as_x()
        - box.position.as_y()
        - box.size.as_y();
    } else if (attributes.flip_x) {
      return pos
        + tileset.tile_size.as_x()
        - tileset.tile_size.as_y()
        - box.position.as_x()
        + box.position.as_y()
        - box.size.as_x();
    }
    return pos
      - tileset.tile_size.as_y()
      + box.position;
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

  static bool set_tile_index(
    Entity& entity,
    uint16_t tile_index,
    Hash<>::Type collision_box_type,
    const World::Boundaries& boundaries,
    bool check_transits
  ) {
    auto& tile = entity.tileset.tiles[tile_index];
    auto& last_tile = entity.tileset.tiles[entity.tile_index];
    bool moved[4] = {false, false, false, false};
    geometry::Vector<float> offset;
  adjust_position:
    auto it = tile.collision_boxes.find(collision_box_type);
    if (it != tile.collision_boxes.end()) {
      for (const auto& pair : it->second) {
        auto box = pair.second;
        auto pos = entity.get_collision_box_position(box) + offset;
        auto pos_box = geometry::Rectangle<float>(pos, box.size);
        geometry::Vector<float> corners[] = {
          pos,
          pos + box.size.as_x(),
          pos + box.size,
          pos + box.size.as_y(),
        };
        size_t last_boxes_count = 0;
        decltype(last_tile.collision_boxes)::const_iterator it;
        if (check_transits) {
          it = last_tile.collision_boxes.find(pair.first);
          if (it != last_tile.collision_boxes.cend()) {
            last_boxes_count = it->second.size();
          }
        }
        geometry::LineSegment<float> transits[last_boxes_count][4];
        bool is_line[last_boxes_count][4];
        if (check_transits && it != last_tile.collision_boxes.cend()) {
          geometry::LineSegment<float>* t = transits[0];
          bool* l = is_line[0];
          for (const auto& pair : it->second) {
            auto box = pair.second;
            auto pos = entity.get_collision_box_position(box) + offset;
            if (std::abs((pos - corners[0]).length())
                < epsilon) {
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
          if (check_transits && last_boxes_count) {
            for (int i = 0; i < last_boxes_count; i++) {
              for (int j = 0; j < 4; j++) {
                if (!can_skip_corner_boundary(j, boundary)
                    && is_line[i][j]) {
                  const auto& t = transits[i][j];
                  if (t.slope() == boundary.slope()) {
                    continue;
                  }
                  auto intersection = boundary.intersection(t, epsilon);
                  if (!intersection.is_nan()) {
                    auto s = intersection - corners[j];
                    if (is_stuck(s, moved)) {
                      return false;
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
            for (int i = 0; i < 4; i++) {
              if (!can_skip_edge_boundary(i, edges[i], boundary)) {
                // Get intersection between new edge and boundary.
                const auto& edge = edges[i];
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
                    auto& curr_boxes = last_tile.collision_boxes;
                    auto it = curr_boxes.find(collision_box_type);
                    bool skip = false;
                    while (it != curr_boxes.end()) {
                      const auto& boxes = it->second;
                      for (auto pair : boxes) {
                        auto box = pair.second;
                        auto pos = entity.get_collision_box_position(box);
                        box.position += pos;
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
                        for (const auto& edge: edges) {
                          if (!boundary.intersection(edge, epsilon).is_nan()) {
                            skip = true;
                            goto boxes_loop_break;
                          }
                        }
                      }
                      it++;
                    }
                  boxes_loop_break:
                    if (skip) {
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
                        if (pos_box.contains(p)) {
                          s = {0, corner.y - p.y};
                        } else {
                          s = {corner.x - p.x, 0};
                        } 
                      } else {
                        if (pos_box.contains(q)) {
                          s = {0, corner.y - q.y};
                        } else {
                          s = {q.x - corner.x, 0};
                        }
                      }
                      break;
                    case 1:
                      if (p.y < q.y) {
                        if (pos_box.contains(p)) {
                          s = {p.x - corner.x, 0};
                        }  else {
                          s = {0, p.y - corner.y};
                        }
                      } else {
                        if (pos_box.contains(q)) {
                          s = {q.x - corner.x, 0};
                        } else {
                          s = {0, q.y - corner.y};
                        }
                      }
                      break;
                    case 2:
                      if (p.x < q.x) {
                        if (pos_box.contains(p)) {
                          s = {0, p.y - corner.y};
                        } else {
                          s = {p.x - corner.x, 0};
                        }
                      } else {
                        if (pos_box.contains(q)) {
                          s = {0, q.y - corner.y};
                        } else {
                          s = {corner.x - q.x, 0};
                        }
                      }
                      break;
                    case 3:
                      if (p.y > q.y) {
                        if (pos_box.contains(p)) {
                          s = {corner.x - p.x, 0};
                        } else {
                          s = {0, corner.y - p.y};
                        }
                      } else {
                        if (pos_box.contains(q)) {
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
                    return false;
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
    }
    entity.tile_index = tile_index;
    entity.position += offset;
    return true;
  }

  Entity::Entity(
    Hash<>::Type collision_box_type,
    const World::Boundaries& boundaries,
    const Tileset& tileset,
    const geometry::Vector<float>& position,
    Attributes attributes,
    Hash<>::Type type,
    uint16_t tile_index
  ) : tileset(tileset),
      position(position),
      attributes(attributes),
      type(type),
      tile_index(0),
      animation({.playing = false}) {
    if (!set_tile_index(
          *this,
          tile_index,
          collision_box_type,
          boundaries,
          false
        )) {
      throw std::runtime_error("Entity: created entity out of bounds");
    }
  }

  Entity::Entity(
    Hash<>::Type collision_box_type,
    const World::Boundaries& boundaries,
    const Tileset& tileset,
    const geometry::Vector<float>& position,
    Attributes attributes,
    Hash<>::Type type,
    AnimationControls animation_controls
  ) : tileset(tileset),
      position(position),
      attributes(attributes),
      type(type),
      tile_index(0) {
    if (!animate(collision_box_type, boundaries, animation_controls)) {
      throw std::runtime_error("Entity: created entity out of bounds");
    }
  }

  Entity::~Entity() {}

  Entity::AnimationControls::AnimationControls() {}

  Entity::AnimationControls::AnimationControls(Hash<>::Type name)
    : name(name),
      loop(true),
      direction(Direction::Normal),
      speed(1) {}

  bool Entity::AnimationControls::operator==(
    const AnimationControls& rhs
  ) const {
    return name == rhs.name
      && loop == rhs.loop
      && direction == rhs.direction
      && speed == rhs.speed;
  }

  Entity::AnimationControls::Builder::Builder()
    : value({
        .name = 0,
        .loop = false,
        .direction = Direction::Normal,
        .speed = 1,
      }) {}

  Entity::AnimationControls::Builder&
  Entity::AnimationControls::Builder::name(Hash<>::Type name) {
    value.name = name;
    return *this;
  }

  Entity::AnimationControls::Builder&
  Entity::AnimationControls::Builder::loop() {
    value.loop = true;
    return *this;
  }

  Entity::AnimationControls::Builder&
  Entity::AnimationControls::Builder::reverse() {
    value.direction = Direction::Reverse;
    return *this;
  }

  Entity::AnimationControls::Builder&
  Entity::AnimationControls::Builder::speed(float speed) {
    value.speed = speed;
    return *this;
  }

  Entity::AnimationControls Entity::AnimationControls::Builder::build() const {
    AnimationControls animation_controls;
    animation_controls.name = value.name;
    animation_controls.loop = value.loop;
    animation_controls.direction = value.direction;
    animation_controls.speed = value.speed;
    return animation_controls;
  }

  bool Entity::animate(
    Hash<>::Type collision_box_type,
    const World::Boundaries& boundaries,
    AnimationControls animation_controls,
    bool force_restart
  ) {
    if (!force_restart && animation_controls == animation.animation_controls) {
      return true;
    }
    auto tile_index = tileset.get_tile_index_by_name(animation_controls.name);
    if (!set_tile_index(
          *this,
          tile_index,
          collision_box_type,
          boundaries,
          true
        )) {
      return false;
    }
    animation.tile = &tileset.tiles.at(tile_index);
    animation.animation_controls = animation_controls;
    animation.counter = 0;
    if (animation.tile->animation_tiles.size()) {
      animation.playing = true;
      switch (animation_controls.direction) {
      case AnimationControls::Direction::Normal:
        animation.animation_tile = animation.tile->animation_tiles.begin();
        break;
      case AnimationControls::Direction::Reverse:
        animation.animation_tile = --animation.tile->animation_tiles.end();
        break;
      }
    } else {
      animation.playing = false;
    }
    return true;
  }

  bool Entity::update_animation(
    Hash<>::Type collision_box_type,
    const World::Boundaries& boundaries
  ) {
    if (animation.playing) {
      if (animation.counter++ == animation.animation_tile->duration) {
        switch (animation.animation_controls.direction) {
        case AnimationControls::Direction::Normal:
          if (std::next(animation.animation_tile)
              == animation.tile->animation_tiles.end()) {
            if (animation.animation_controls.loop) {
              animation.animation_tile =
                animation.tile->animation_tiles.begin();
            } else {
              animation.playing = false;
            }
          } else {
            animation.animation_tile++;
          }
          break;
        case AnimationControls::Direction::Reverse:
          if (std::prev(animation.animation_tile)
              == animation.tile->animation_tiles.begin()) {
            if (animation.animation_controls.loop) {
              animation.animation_tile =
                std::prev(animation.tile->animation_tiles.end());
            } else {
              animation.playing = false;
            }
          } else {
            animation.animation_tile--;
          }
          break;
        }
        animation.counter = 0;
        return set_tile_index(
          *this,
          animation.animation_tile->tile_index,
          collision_box_type,
          boundaries,
          true
        );
      }
    }
    return true;
  }

  void Entity::update(
    World::Boundaries& boundaries,
    Entity* player,
    const std::vector<Entity*>& entities
  ) {
  }

  std::pair<bool, Entity::BoundaryCollision> Entity::get_boundary_collision(
    geometry::Vector<float> force,
    const Tileset::Tile::CollisionBox::NamedList& collision_boxes,
    const World::Boundaries& boundaries
  ) {
    BoundaryCollision closest = {{.distance = geometry::Vector<float>::NaN()}};
    if (force.x != 0 || force.y != 0) {
      for (auto pair : collision_boxes) {
        auto name = pair.first;
        auto box = pair.second;
        auto pos = get_collision_box_position(box);
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
                    closest.edge = Entity::Collision::Edge::Left;
                  } else {
                    closest.edge = Entity::Collision::Edge::Top;
                  }
                  break;
                case 1:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Entity::Collision::Edge::Right;
                  } else {
                    closest.edge = Entity::Collision::Edge::Top;
                  }
                  break;
                case 2:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Entity::Collision::Edge::Right;
                  } else {
                    closest.edge = Entity::Collision::Edge::Bottom;
                  }
                  break;
                case 3:
                  if (curr->slope()
                      == std::numeric_limits<float>::infinity()) {
                    closest.edge = Entity::Collision::Edge::Left;
                  } else {
                    closest.edge = Entity::Collision::Edge::Bottom;
                  }
                  break;
                }
                closest.name = name;
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
          auto pos = position
            - tileset.tile_size.as_y()
            + box.position;
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
                  closest.edge = Entity::Collision::Edge::Left;
                  closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Right;
                  closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Top;
                  closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Bottom;
                  closest.name = name;
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
                closest.edge = Entity::Collision::Edge::Top;
                closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Left;
                  closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Right;
                  closest.name = name;
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
                closest.edge = Entity::Collision::Edge::Bottom;
                closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Left;
                  closest.name = name;
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
                  closest.edge = Entity::Collision::Edge::Right;
                  closest.name = name;
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

}
