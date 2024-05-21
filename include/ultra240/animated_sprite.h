#pragma once

#include <vector>
#include <ultra240/sprite.h>
#include <ultra240/tileset.h>

namespace ultra {

  /** An animated sprite. */
  class AnimatedSprite : public Sprite {
  public:

    /** Structure defining animation controls. */
    struct AnimationControls {

      /** Flag that toggles animation looping. */
      bool loop;

      /** Signal defining how animation frames advance. */
      enum Direction {

        /** Animation tiles are incremented. */
        Normal,

        /** Animation tiles are decremented. */
        Reverse,

      } direction;

      /** Name of target animation. */
      Hash name;

      /** Animation speed multiplier. */
      float speed;

      /** Instance constructor. */
      AnimationControls();

      /** Instance constructor with default controls. */
      AnimationControls(Hash name);

      /** Instance compare operator. */
      bool operator==(const AnimationControls& rhs) const;

      /** Instance builder. */
      class Builder {

        struct {
          Hash name;
          bool loop;
          Direction direction;
          float speed;
        } value;

      public:

        /** Builder instance constructor. */
        Builder();

        /** Set the name of the target animation. */
        Builder& name(Hash name);

        /** Enable animation looping. */
        Builder& loop();

        /** Set reverse animation direction. */
        Builder& reverse();

        /** Set animation speed. */
        Builder& speed(float speed);

        /** Return built instance. */
        AnimationControls build() const;
      };
    };

    /** Class defining an animation. */
    class Animation {
    public:

      /** Shorthand for Tileset's AnimationTile. */
      using AnimationTile = Tileset::Tile::AnimationTile;

      /** Animation set constructor. */
      Animation(
        const ultra::Tileset& tileset,
        uint16_t tile_index,
        AnimationControls animation_controls
      );

      /** Copy constructor. */
      Animation(const Animation& animation);

      /** Copy operator. */
      Animation& operator=(const Animation& rhs);

      /**
       * Return new animation based on specified controls. Will return current
       * animation if the target animation is the current animation unless
       * specified.
       */
      Animation set(
        AnimationControls controls,
        bool force_restart
      );

      /** Return updated animation. */
      Animation update();

      /** 
       * Get the current tile index of the animation.
       */
      uint16_t get_tile_index() const;

      /** True if current animation is playing. */
      bool playing;

      /** The tileset the animaiton belongs to. */
      const ultra::Tileset* tileset;

      /** Pointer to the animation tile data. */
      const Tileset::Tile* tile;

      /** The animation controls for the current animation. */
      AnimationControls animation_controls;

      /** Frame counter for the current animation tile. */
      uint32_t counter;

      /** Iterator to the current animation tile. */
      union Iterator {
        Iterator() {}
        std::vector<AnimationTile>::const_iterator normal;
        std::vector<AnimationTile>::const_reverse_iterator reverse;
        uint16_t tile_index;
      } iterator;
    };

    /** Instance constructor. */
    AnimatedSprite(
      const Tileset& tileset,
      const geometry::Vector<float>& position,
      Tileset::Attributes attributes,
      uint16_t tile_index,
      float transform[9] = nullptr
    );

    /**
     * Set current animation. Will not restart if the target animation
     * is the current animation unless specified.
     */
    void animate(
      AnimationControls controls,
      bool force_restart = false
    );

    /** Update the current animation tile. */
    void update_animation();

    /** The current animation. */
    Animation animation;
  };

}
