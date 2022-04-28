struct Core {
    Array<struct GameObject> go_data;
    Array<struct GoRenderUnit> go_render;
    Array<struct ParticleEmitter> particle_emitters;
    Array<struct ParticleRenderUnit> particle_render;
};

static void core_init(Core *core) {
    core->go_data = arr_new<struct GameObject>(10);
    core->go_render = arr_new<struct GoRenderUnit>(10);
    core->particle_emitters = arr_new<struct ParticleEmitter>(10);
    core->particle_render = arr_new<struct ParticleRenderUnit>(10);
}

static void core_deinit(Core *core) {
    arr_deinit<struct GameObject>(&core->go_data);
    arr_deinit<struct GoRenderUnit>(&core->go_render);
    arr_deinit<struct ParticleEmitter>(&core->particle_emitters);
    arr_deinit<struct ParticleRenderUnit>(&core->particle_render);
    free(core);
}

// NOTE: The rest of the system depends on this. If you want to put any functionality here, think twice
