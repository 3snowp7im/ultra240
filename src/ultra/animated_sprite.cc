#include <stdexcept>
#include <ultra240/animated_sprite.h>

namespace ultra {

  AnimatedSprite::AnimationControls::AnimationControls()
    : loop(true),
      direction(Direction::Normal),
      speed(1) {}

  bool AnimatedSprite::AnimationControls::operator==(
    const AnimationControls& rhs
  ) const {
    return loop == rhs.loop
      && direction == rhs.direction
      && speed == rhs.speed;
  }

  AnimatedSprite::AnimationControls::Builder::Builder()
    : value({
        .loop = false,
        .direction = Direction::Normal,
        .speed = 1,
      }) {}

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
    animation_controls.loop = value.loop;
    animation_controls.direction = value.direction;
    animation_controls.speed = value.speed;
    return animation_controls;
  }

  AnimatedSprite::AnimatedSprite(
    const Tileset& tileset,
    Hash name,
    const AnimationControls& animation_controls,
    const geometry::Vector<float>& position,
    Tileset::Attributes attributes,
    float transform[9]
  ) : animation(tileset, name, animation_controls),
      ultra::Sprite(
        tileset,
        tileset.get_tile_index_by_name(name),
        position,
        attributes,
        transform
      ) {}

  AnimatedSprite::AnimatedSprite(
    const Tileset& tileset,
    uint16_t tile_index,
    const geometry::Vector<float>& position,
    const Tileset::Attributes& attributes,
    float transform[9]
  ) : animation(tileset, tile_index),
      ultra::Sprite(
        tileset,
        tile_index,
        position,
        attributes,
        transform
      ) {}

  static void animation_construct(
    AnimatedSprite::Animation& animation,
    uint16_t tile_index
  ) {
    if (animation.tile->animation_tiles.size()) {
      animation.playing = true;
      switch (animation.animation_controls.direction) {
      case AnimatedSprite::AnimationControls::Direction::Normal:
        animation.iterator.normal = animation.tile->animation_tiles.cbegin();
        break;
      case AnimatedSprite::AnimationControls::Direction::Reverse:
        animation.iterator.reverse = animation.tile->animation_tiles.crbegin();
        break;
      }
    } else {
      animation.playing = false;
      animation.iterator.tile_index = tile_index;
    }
  }

  AnimatedSprite::Animation::Animation(
    const ultra::Tileset& tileset,
    Hash name,
    const AnimatedSprite::AnimationControls& animation_controls
  ) : name(name),
      tileset(&tileset),
      animation_controls(animation_controls),
      counter(0) {
    uint16_t tile_index = tileset.get_tile_index_by_name(name);
    tile = &tileset.tiles[tile_index];
    animation_construct(*this, tile_index);
  }

  AnimatedSprite::Animation::Animation(
    const ultra::Tileset& tileset,
    uint16_t tile_index
  ) : name(0),
      tileset(&tileset),
      counter(0) {
    tile = &tileset.tiles[tile_index];
    animation_construct(*this, tile_index);
  }

  AnimatedSprite::Animation::Animation(
    const Animation& animation
  ) : name(animation.name),
      tileset(animation.tileset),
      animation_controls(animation.animation_controls),
      counter(animation.counter),
      tile(animation.tile),
      playing(animation.playing) {
    if (animation.tile->animation_tiles.size()) {
      switch (animation.animation_controls.direction) {
      case AnimatedSprite::AnimationControls::Direction::Normal:
        iterator.normal = animation.iterator.normal;
        break;
      case AnimatedSprite::AnimationControls::Direction::Reverse:
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
    name = rhs.name;
    playing = rhs.playing;
    tileset = rhs.tileset;
    tile = rhs.tile;
    animation_controls = rhs.animation_controls;
    counter = rhs.counter;
    if (rhs.tile->animation_tiles.size()) {
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
    Hash name,
    AnimationControls animation_controls,
    bool force_restart
  ) {
    if (!force_restart
        && name == this->name
        && animation_controls == this->animation_controls) {
      return *this;
    }
    return Animation(*tileset, name, animation_controls);
  }

  template <typename T>
  static void advance(
    AnimatedSprite::Animation& animation,
    T& iterator,
    T begin,
    T end
  ) {
    uint32_t duration = animation.animation_controls.speed * iterator->duration;
    if (animation.counter++ == duration) {
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
    Hash name,
    AnimationControls animation_controls,
    bool force_restart
  ) {
    animation = animation.set(name, animation_controls, force_restart);
    tile_index = animation.get_tile_index();
  }

  void AnimatedSprite::update_animation() {
    animation = animation.update();
    tile_index = animation.get_tile_index();
  }

}
