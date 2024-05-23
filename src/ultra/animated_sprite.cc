#include <stdexcept>
#include <ultra240/animated_sprite.h>

namespace ultra {

  AnimatedSprite::Controls::Controls()
    : loop(true),
      direction(Direction::Normal),
      speed(1) {}

  bool AnimatedSprite::Controls::operator==(
    const Controls& rhs
  ) const {
    return loop == rhs.loop
      && direction == rhs.direction
      && speed == rhs.speed;
  }

  AnimatedSprite::Controls::Builder::Builder()
    : value({
        .loop = false,
        .direction = Direction::Normal,
        .speed = 1,
      }) {}

  AnimatedSprite::Controls::Builder&
  AnimatedSprite::Controls::Builder::loop() {
    value.loop = true;
    return *this;
  }

  AnimatedSprite::Controls::Builder&
  AnimatedSprite::Controls::Builder::reverse() {
    value.direction = Direction::Reverse;
    return *this;
  }

  AnimatedSprite::Controls::Builder&
  AnimatedSprite::Controls::Builder::speed(float speed) {
    value.speed = speed;
    return *this;
  }

  AnimatedSprite::Controls
  AnimatedSprite::Controls::Builder::build() const {
    Controls controls;
    controls.loop = value.loop;
    controls.direction = value.direction;
    controls.speed = value.speed;
    return controls;
  }

  AnimatedSprite::AnimatedSprite(
    const Tileset& tileset,
    Hash name,
    const Controls& controls,
    const geometry::Vector<float>& position,
    Tileset::Attributes attributes,
    float transform[9]
  ) : animation(tileset, name, controls),
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
      switch (animation.controls.direction) {
      case AnimatedSprite::Controls::Direction::Normal:
        animation.iterator.normal = animation.tile->animation_tiles.cbegin();
        break;
      case AnimatedSprite::Controls::Direction::Reverse:
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
    const AnimatedSprite::Controls& controls
  ) : name(name),
      tileset(&tileset),
      controls(controls),
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
      controls(animation.controls),
      counter(animation.counter),
      tile(animation.tile),
      playing(animation.playing) {
    if (animation.tile->animation_tiles.size()) {
      switch (animation.controls.direction) {
      case AnimatedSprite::Controls::Direction::Normal:
        iterator.normal = animation.iterator.normal;
        break;
      case AnimatedSprite::Controls::Direction::Reverse:
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
    controls = rhs.controls;
    counter = rhs.counter;
    if (rhs.tile->animation_tiles.size()) {
      switch (rhs.controls.direction) {
      case Controls::Direction::Normal:
        iterator.normal = rhs.iterator.normal;
        break;
      case Controls::Direction::Reverse:
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
    const Controls& controls,
    bool force_restart
  ) {
    if (!force_restart
        && name == this->name
        && controls == this->controls) {
      return *this;
    }
    return Animation(*tileset, name, controls);
  }

  template <typename T>
  static void advance(
    AnimatedSprite::Animation& animation,
    T& iterator,
    T begin,
    T end
  ) {
    uint32_t duration = animation.controls.speed * iterator->duration;
    if (animation.counter++ == duration) {
      if (std::next(iterator) == end) {
        if (animation.controls.loop) {
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
      switch (controls.direction) {
      case Controls::Direction::Normal:
        advance(
          next,
          next.iterator.normal,
          next.tile->animation_tiles.cbegin(),
          next.tile->animation_tiles.cend()
        );
        break;
      case Controls::Direction::Reverse:
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
      switch (controls.direction) {
      case Controls::Direction::Normal:
        return iterator.normal->tile_index;
      case Controls::Direction::Reverse:
        return iterator.reverse->tile_index;
      }
    }
    return iterator.tile_index;
  }

  void AnimatedSprite::animate(
    Hash name,
    const Controls& controls,
    bool force_restart
  ) {
    animation = animation.set(name, controls, force_restart);
    tile_index = animation.get_tile_index();
  }

  void AnimatedSprite::update_animation() {
    animation = animation.update();
    tile_index = animation.get_tile_index();
  }

}
