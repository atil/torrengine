struct Core {
    Array<struct GameObject> go_data;
    Array<struct GoRenderUnit> go_render;
    Array<struct ParticleSource> particle_sources;
    Array<struct ParticleRenderUnit> particle_render;
    Array<struct Widget> ui_widgets;
    Array<struct UiRenderUnit> ui_render;
};

static void core_init(Core *core) {
    core->go_data = arr_new<struct GameObject>(10);
    core->go_render = arr_new<struct GoRenderUnit>(10);
    core->particle_sources = arr_new<struct ParticleSource>(10);
    core->particle_render = arr_new<struct ParticleRenderUnit>(10);
    core->ui_widgets = arr_new<struct Widget>(10);
    core->ui_render = arr_new<struct UiRenderUnit>(10);
}

static void core_deinit(Core *core) {
    arr_deinit<struct GameObject>(&core->go_data);
    arr_deinit<struct GoRenderUnit>(&core->go_render);
    arr_deinit<struct ParticleSource>(&core->particle_sources);
    arr_deinit<struct ParticleRenderUnit>(&core->particle_render);
    arr_deinit<struct Widget>(&core->ui_widgets); // TODO @LEAK: This leaks the string inside
    arr_deinit<struct UiRenderUnit>(&core->ui_render);
    free(core);
}

// NOTE: The rest of the system depends on this. If you want to put any functionality here, think twice
