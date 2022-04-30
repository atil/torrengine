struct Core {
    Array<struct GameObject> go_data;
    Array<struct GoRenderUnit> go_render;
    Array<struct ParticleSource> particle_sources;
    Array<struct ParticleRenderUnit> particle_render;
    Array<struct Widget> ui_widgets;
    Array<struct UiRenderUnit> ui_render;
};

static void particle_source_deinit(struct ParticleSource *source);
static void render_unit_particle_deinit(struct ParticleRenderUnit *ru);
static void widget_deinit(struct Widget *widget);
static void render_unit_ui_deinit(struct UiRenderUnit *ru);

static void core_init(Core *core) {
    core->go_data = arr_new<struct GameObject>(10, nullptr);
    core->go_render = arr_new<struct GoRenderUnit>(10, nullptr);
    core->particle_sources = arr_new<struct ParticleSource>(10, &particle_source_deinit);
    core->particle_render = arr_new<struct ParticleRenderUnit>(10, &render_unit_particle_deinit);
    core->ui_widgets = arr_new<struct Widget>(10, &widget_deinit);
    core->ui_render = arr_new<struct UiRenderUnit>(10, &render_unit_ui_deinit);
}

static void core_deinit(Core *core) {
    arr_deinit<struct GameObject>(&core->go_data);
    arr_deinit<struct GoRenderUnit>(&core->go_render);
    arr_deinit<struct ParticleSource>(&core->particle_sources);
    arr_deinit<struct ParticleRenderUnit>(&core->particle_render);
    arr_deinit<struct Widget>(&core->ui_widgets); // TODO @LEAK: This leaks the strings inside
    arr_deinit<struct UiRenderUnit>(&core->ui_render);
    free(core);
}

// NOTE: The rest of the system depends on this. If you want to put any functionality here, think twice
