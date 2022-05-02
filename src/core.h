struct Core {
    Array<struct GameObject> go_data;
    Array<struct GoRenderUnit> go_render;
    Array<struct ParticleSource> particle_sources;
    Array<struct ParticleRenderUnit> particle_render;
    Array<struct Widget> ui_widgets;
    Array<struct UiRenderUnit> ui_render;

    Core() : go_data(10), go_render(10), particle_sources(10), particle_render(10), ui_widgets(10), ui_render(10) {
    }

    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(const Core &&) = delete;
};

