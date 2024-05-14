#pragma once

#include <ultra240/hash.h>
#include <ultra240/sprite.h>

namespace ultra {

  struct AnimatedSprite : public Sprite {

    /**
     * Structure defining controls of sprite animations.
     */ 
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

      /** Instance compare operator. */
      bool operator==(const AnimationControls& rhs) const;

      /** Name of target animation. */
      Hash name;

      /** Animation speed multiplier. */
      float speed;

      /** Instance constructor. */
      AnimationControls();

      /** Instance constructor with default controls. */
      AnimationControls(Hash name);

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

    /**
     * Set sprite's current animation. Will not restart if the target animation
     * is the current animation unless specified.
     */
    virtual bool animate(
      Hash collision_box_type,
      const World::Boundaries& boundaries,
      AnimationControls animation_controls,
      bool force_restart = false
    );

    /** Update the current animation tile. */
    virtual bool update_animation(
      Hash collision_box_type,
      const World::Boundaries& boundaries
    );

  }
