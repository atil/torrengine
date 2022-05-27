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

    Core() {

        // TODO @DEBT @ROBUSTNESS: As soon as this reservation is reached and the vector need to do further
        // allocation, it calls the destructors for its members. We need to prevent that by either replacing
        // std::vector with our own, or using another (custom) collection for the struct which should never
        // destroyed implicitly (like during allocation)

        go_data.reserve(10);
        go_render.reserve(10);
        particle_sources.reserve(10);
        particle_render.reserve(10);
        ui_widgets.reserve(10);
        ui_render.reserve(10);
    }

    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(const Core &&) = delete;
};
