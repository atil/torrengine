struct EntityId {
    EntityIndex index;
    String tag;

    // NOTE @BUGFIX: Default ctor isn't auto-generated as soon as we define _any_ ctor. Because of the explicit
    // ctor below, we need this manual default ctor
    EntityId() : index(0), tag() {
    }

    explicit EntityId(EntityIndex i, String tag_arg) {
        index = i;
        tag = tag_arg;
    }
};

struct Core {
    Array<struct GameObject> go_data;
    Array<struct GoRenderUnit> go_render;
    Array<struct ParticleSource> particle_sources;
    Array<struct ParticleRenderUnit> particle_render;
    Array<struct Widget> ui_widgets;
    Array<struct UiRenderUnit> ui_render;

    Array<EntityId> entities; // This is gonna replace PongEntities

    Core()
        : go_data(10), go_render(10), particle_sources(10), particle_render(10), ui_widgets(10), ui_render(10),
          entities(100) {
    }

    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(const Core &&) = delete;

    EntityIndex get_entity_by_tag(String *tag); // TODO @INCOMPLETE
};

