enum class ComponentType
{
    GameObject = 0,
    GoRender,
    ParticleData,
    ParticleRender,
    UiData,
    UiRender,
};

struct Entity {
    u32 id;
    u32 mask;
};

template <typename T>
struct ComponentArray {
    Array<T> components;
    IndexMap entity_to_index;
    // TODO @INCOMPLETE: How do we go from the component to the entity?

    const usize max_components = 128; // TODO @CLEANUP: Find a proper number

    explicit ComponentArray() : components(max_components), entity_to_index(max_components) {
    }

    // We send a reference to keep the component's possible pointers alive
    void register_component(Entity e, const T &component) {
        components.add(component);
        entity_to_index.add(e.id, (u32)components.count);
    }

    T *get_component(Entity e) {
        u32 comp_index = entity_to_index.get(e.index);
        return components[comp_index];
    }

    void remove_component(Entity e) {
        u32 comp_index = entity_to_index.get(e.index);
        T *component = components[comp_index];
        components.remove(component);
        entity_to_index.remove(e.index);
    }

    ComponentArray(const ComponentArray &) = delete;
    ComponentArray(ComponentArray &&) = delete;
    void operator=(const ComponentArray &) = delete;
    void operator=(ComponentArray &&) = delete;
};

struct Core {
    Array<struct GameObject> go_data;                 // 0
    Array<struct GoRenderUnit> go_render;             // 1
    Array<struct ParticleSource> particle_sources;    // 2
    Array<struct ParticleRenderUnit> particle_render; // 3
    Array<struct Widget> ui_widgets;                  // 4
    Array<struct UiRenderUnit> ui_render;             // 5

    Array<Entity> entities; // This is gonna replace PongEntities

    ComponentArray<struct GameObject> go_data_comps;
    ComponentArray<struct GoRenderUnit> go_render_comps;
    ComponentArray<struct ParticleSource> particle_data_comps;
    ComponentArray<struct ParticleRenderUnit> particle_render_comps;
    ComponentArray<struct Widget> ui_data_comps;
    ComponentArray<struct UiRenderUnit> ui_render_comps;

    u32 next_entity_id = 0;
    u8 _padding[4];

    Core()
        : go_data(10), go_render(10), particle_sources(10), particle_render(10), ui_widgets(10), ui_render(10),
          entities(100) {
    }

    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(const Core &&) = delete;

    Entity generate_entity() {
        Entity e;
        e.id = next_entity_id;
        next_entity_id++;
        return e;
    }
};

