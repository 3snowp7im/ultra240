#include <stdexcept>
#include <ultra240/animated_sprite.h>

namespace ultra {

  AnimatedSprite::AnimationControls::AnimationControls()
    : name(0),
      loop(false),
      direction(Direction::Normal),
      speed(1) {}

  AnimatedSprite::AnimationControls::AnimationControls(Hash name)
    : name(name),
      loop(true),
      direction(Direction::Normal),
      speed(1) {}

  bool AnimatedSprite::AnimationControls::operator==(
    const AnimationControls& rhs
  ) const {
    return name == rhs.name
      && loop == rhs.loop
      && direction == rhs.direction
      && speed == rhs.speed;
  }

  AnimatedSprite::AnimationControls::Builder::Builder()
    : value({
        .name = 0,
        .loop = false,
        .direction = Direction::Normal,
        .speed = 1,
      }) {}

  AnimatedSprite::AnimationControls::Builder&
  AnimatedSprite::AnimationControls::Builder::name(Hash name) {
    value.name = name;
    return *this;
  }

  AnimatedSprite::AnimationControls::Builder&
  AnimatedSprite::AnimationControls::Builder::loop() {
    value.loop = true;
    return *this;
  }

  AnimatedSprite::AnimationControls::Builder&
  AnimatedSprite::AnimationControls::Builder::reverse() {
    value.direction = Direction::Reverse;
    return *this;
  }

  AnimatedSprite::AnimationControls::Builder&
  AnimatedSprite::AnimationControls::Builder::speed(float speed) {
    value.speed = speed;
    return *this;
  }

  AnimatedSprite::AnimationControls
  AnimatedSprite::AnimationControls::Builder::build() const {
    AnimationControls animation_controls;
    animation_controls.name = value.name;
    animation_controls.loop = value.loop;
    animation_controls.direction = value.direction;
    animation_controls.speed = value.speed;
    return animation_controls;
  }

  AnimatedSprite::AnimatedSprite(
    const Tileset& tileset,
    const geometry::Vector<float>& position,
    Tileset::Attributes attributes,
    uint16_t tile_index,
    float transform[9]
  ) : animation(
        tileset,
        tile_index,
        ultra::AnimatedSprite::AnimationControls()
      ),
      ultra::Sprite(tileset, position, attributes, tile_index, transform) {}

  AnimatedSprite::Animation::Animation(
    const ultra::Tileset& tileset,
    uint16_t tile_index,
    AnimationControls animation_controls
  ) : tileset(&tileset),
      tile(&tileset.tiles[tile_index]),
      animation_controls(animation_controls),
      counter(0) {
    if (tile->animation_tiles.size()) {
      playing = true;
      switch (animation_controls.direction) {
      case AnimationControls::Direction::Normal:
        iterator.normal = tile->animation_tiles.cbegin();
        break;
      case AnimationControls::Direction::Reverse:
        iterator.reverse = tile->animation_tiles.crbegin();
        break;
      }
    } else {
      playing = false;
      iterator.tile_index = tile_index;
    }
  }

  AnimatedSprite::Animation::Animation(
    const Animation& animation
  ) : playing(animation.playing),
      tileset(animation.tileset),
      tile(animation.tile),
      animation_controls(animation.animation_controls),
      counter(animation.counter) {
    if (tile->animation_tiles.size()) {
      switch (animation.animation_controls.direction) {
      case AnimationControls::Direction::Normal:
        iterator.normal = animation.iterator.normal;
        break;
      case AnimationControls::Direction::Reverse:
        iterator.reverse = animation.iterator.reverse;
        break;
      }
    } else {
      iterator.tile_index = animation.iterator.tile_index;
    }
  }

  AnimatedSprite::Animation& AnimatedSprite::Animation::operator=(
    const Animation& rhs
  ) {
    playing = rhs.playing;
    tileset = rhs.tileset;
    tile = rhs.tile;
    animation_controls = rhs.animation_controls;
    counter = rhs.counter;
    if (tile->animation_tiles.size()) {
      switch (rhs.animation_controls.direction) {
      case AnimationControls::Direction::Normal:
        iterator.normal = rhs.iterator.normal;
        break;
      case AnimationControls::Direction::Reverse:
        iterator.reverse = rhs.iterator.reverse;
        break;
      }
    } else {
      iterator.tile_index = rhs.iterator.tile_index;
    }
    return *this;
  }

  AnimatedSprite::Animation AnimatedSprite::Animation::set(
    AnimationControls animation_controls,
    bool force_restart
  ) {
    if (!force_restart && animation_controls == this->animation_controls) {
      return *this;
    }
    auto tile_index = tileset->get_tile_index_by_name(animation_controls.name);
    return Animation(
      *tileset,
      tile_index,
      animation_controls
    );
  }

  template <typename T>
  static void advance(
    AnimatedSprite::Animation& animation,
    T& iterator,
    T begin,
    T end
  ) {
    if (animation.counter++ == iterator->duration) {
      if (std::next(iterator) == end) {
        if (animation.animation_controls.loop) {
          iterator = begin;
        } else {
          animation.playing = false;
        }
      } else {
        iterator++;
      }
      animation.counter = 0;
    }
  }

  AnimatedSprite::Animation AnimatedSprite::Animation::update() {
    Animation next(*this);
    if (playing && tile->animation_tiles.size()) {
      switch (animation_controls.direction) {
      case AnimationControls::Direction::Normal:
        advance(
          next,
          next.iterator.normal,
          next.tile->animation_tiles.cbegin(),
          next.tile->animation_tiles.cend()
        );
        break;
      case AnimationControls::Direction::Reverse:
        advance(
          next,
          next.iterator.reverse,
          next.tile->animation_tiles.crbegin(),
          next.tile->animation_tiles.crend()
        );
        break;
      }
    }
    return next;
  }

  uint16_t AnimatedSprite::Animation::get_tile_index() const {
    if (tile->animation_tiles.size()) {
      switch (animation_controls.direction) {
      case AnimationControls::Direction::Normal:
        return iterator.normal->tile_index;
      case AnimationControls::Direction::Reverse:
        return iterator.reverse->tile_index;
      }
    }
    return iterator.tile_index;
  }

  void AnimatedSprite::animate(
    AnimationControls animation_controls,
    bool force_restart
  ) {
    animation = animation.set(animation_controls, force_restart);
    tile_index = animation.get_tile_index();
  }

  void AnimatedSprite::update_animation() {
    animation = animation.update();
    tile_index = animation.get_tile_index();
  }

}
