    /**
     * Structure defining controls of entity animations.
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
     * Set entity's current animation. Will not restart if the target animation
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

    /** The current animation settings. */
    struct {

      /** True if current animation is playing. */
      bool playing;

      /** Pointer to the current animation's tiles. */
      const std::vector<Tileset::Tile::AnimationTile>* animation_tiles;

      /** The animation controls for the current animation. */
      AnimationControls animation_controls;

      /** Frame counter for the current animation tile. */
      uint32_t counter;

      /** Iterator to the current animation tile. */
      std::vector<Tileset::Tile::AnimationTile>::const_iterator animation_tile;

    } animation;
