struct Core {
    Array<GameObject> go_data;
    Array<GoRenderUnit> go_render;
    Array<ParticleEmitter> particle_emitters;
    Array<ParticleRenderUnit> particle_render;
};

static void core_init(Core *core) {
    core->go_data = arr_new<GameObject>(10);
    core->go_render = arr_new<GoRenderUnit>(10);
    core->particle_emitters = arr_new<ParticleEmitter>(10);
    core->particle_render = arr_new<ParticleRenderUnit>(10);
}

static void core_deinit(Core *core) {
    arr_deinit(core->go_data);
    arr_deinit(core->go_render);
    arr_deinit(core->go_particle_emitters);
    arr_deinit(core->go_particle_render);
    free(core);
}

