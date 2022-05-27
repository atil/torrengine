#define NONCOPYABLE(Type)                                                                                         \
    Type(const Type &) = delete;                                                                                  \
    Type(Type &&) = delete;                                                                                       \
    Type &operator=(const Type &) = delete;                                                                       \
    Type &operator=(Type &&) = delete;

struct Core {
    std::vector<struct GoData> go_data;
    std::vector<struct GoRenderUnit> go_render;
    std::vector<struct ParticleSource> particle_sources;
    std::vector<struct ParticleRenderUnit> particle_render;
    std::vector<struct Widget> ui_widgets;
    std::vector<struct UiRenderUnit> ui_render;

    Core() = default;

    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(const Core &&) = delete;
};
