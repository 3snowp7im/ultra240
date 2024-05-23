#pragma once

#include <vector>
#include <ultra240/sprite.h>
#include <ultra240/tileset.h>

namespace ultra {

  /** An animated sprite. */
  class AnimatedSprite : public Sprite {
  public:

    /** Structure defining animation controls. */
    struct Controls {

      /** Flag that toggles animation looping. */
      bool loop;

      /** Signal defining how animation frames advance. */
      enum Direction {

        /** Animation tiles are incremented. */
        Normal,

        /** Animation tiles are decremented. */
        Reverse,

      } direction;

      /** Animation speed multiplier. */
      float speed;

      /** Instance constructor. */
      Controls();

      /** Instance compare operator. */
      bool operator==(const Controls& rhs) const;

      /** Instance builder. */
      class Builder {

        struct {
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
        Controls build() const;
      };
    };

    /** Class defining an animation. */
    class Animation {
    public:

      /** Shorthand for Tileset's AnimationTile. */
      using AnimationTile = Tileset::Tile::AnimationTile;

      /** Static tile index constructor. */
      Animation(
        const ultra::Tileset& tileset,
        uint16_t tile_index
      );

      /** Animation set constructor. */
      Animation(
        const ultra::Tileset& tileset,
        Hash name,
        const Controls& controls
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
        Hash name,
        const Controls& controls,
        bool force_restart
      );

      /** Return updated animation. */
      Animation update();

      /** 
       * Get the current tile index of the animation.
       */
      uint16_t get_tile_index() const;

      /** Name of the animation. */
      Hash name;

      /** True if current animation is playing. */
      bool playing;

      /** The tileset the animaiton belongs to. */
      const ultra::Tileset* tileset;

      /** Pointer to the animation tile data. */
      const Tileset::Tile* tile;

      /** The animation controls for the current animation. */
      Controls controls;

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

    /** Animation constructor. */
    AnimatedSprite(
      const Tileset& tileset,
      Hash name,
      const Controls& animation_controls,
      const geometry::Vector<float>& position,
      Tileset::Attributes attributes,
      float transform[9] = nullptr
    );

    /** Tileset constructor. */
    AnimatedSprite(
      const Tileset& tileset,
      uint16_t tile_index,
      const geometry::Vector<float>& position,
      const Tileset::Attributes& attributes,
      float transform[9] = nullptr
    );

    /**
     * Set current animation. Will not restart if the target animation
     * is the current animation unless specified.
     */
    void animate(
      Hash name,
      const Controls& controls,
      bool force_restart = false
    );

    /** Update the current animation tile. */
    void update_animation();

    /** The current animation. */
    Animation animation;
  };

}
