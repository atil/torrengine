struct Core {
    Array<struct GameObject> go_data;
    Array<struct GoRenderUnit> go_render; // TODO @LEAK: deinit() isn't called. Make this dtor
    Array<struct ParticleSource> particle_sources;
    Array<struct ParticleRenderUnit> particle_render; // TODO @LEAK: deinit() isn't called. Make this dtor
    Array<struct Widget> ui_widgets;
    Array<struct UiRenderUnit> ui_render; // TODO @LEAK: deinit() isn't called. Make this dtor

    Core() : go_data(10), go_render(10), particle_sources(10), particle_render(10), ui_widgets(10), ui_render(10) {
    }

    Core(const Core &) = delete;
    Core(Core &&) = delete;
    Core &operator=(const Core &) = delete;
    Core &operator=(const Core &&) = delete;
};
