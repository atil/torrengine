// tasks:
// - continue implement map
// - we should get rid of pointer types in components. it'll be a lot clearer if they're dumb data
// - make the engine a library. in which way do we divide things into its own modules?
// - write down which structs need ctor/dtor, i.e. the ones that can be array elements
// - implement Array::add_move(). deepcopy'ing stuff every time we add to array is bad
// - rename: deinit->dispose
// - mat4 shenanigans

// NOTE @DOCS: Game origin: up-left

#define GLEW_STATIC                 // Statically linking glew
#define GLFW_DLL                    // Dynamically linking glfw
#define GLFW_INCLUDE_NONE           // Disable including dev environment header
#define STB_TRUETYPE_IMPLEMENTATION // stb requires these
#define STB_IMAGE_IMPLEMENTATION

// Disable a bunch of warnings from system headers
#pragma warning(push, 0)
#pragma warning(disable : 5040) // These require extra attention for some reason
#pragma warning(disable : 5045) // Spectre thing
#include <GL/glew.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_truetype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <vector>
#pragma warning(pop)

#define WIDTH 640
#define HEIGHT 480

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#pragma warning(disable : 4505) // Unreferenced functions

// Core module (lib)
#include "types.h"
#include "container.h"
#include "util.h"
#include "tomath.h"

// Engine module
#include "core.h"
#include "input.h"
#include "shader.h"
#include "ui.h"
#include "render.h"
#include "particle.h"
#include "sfx.h"

// Game module

// TODO @CLEANUP: How to deal with this? Move to its own file?
struct PongEntities {
    EntityIndex entity_ui_splash = 0;
    EntityIndex entity_ui_score = 1;
    EntityIndex entity_ui_intermission = 2;

    EntityIndex entity_world_field = 0;
    EntityIndex entity_world_pad1 = 1;
    EntityIndex entity_world_pad2 = 2;
    EntityIndex entity_world_ball = 3;
};

EntityIndex register_particle(Core *core, ParticleProps *props, Renderer *renderer, Vec2 emit_point) {
    shader_handle_t particle_shader = load_shader("src/world.glsl"); // Using world shader for now
    ParticleSource source = particle_source_init(props, emit_point);
    ParticleRenderUnit ru = render_unit_particle_init(props->count, particle_shader, "assets/Ball.png");
    source.isAlive = true; // TODO @INCOMPLETE: We might want make this alive later

    glUseProgram(ru.shader);
    Mat4 mat_identity = mat4_identity();
    shader_set_mat4(ru.shader, "u_model", &mat_identity);
    shader_set_mat4(ru.shader, "u_view", &(renderer->view));
    shader_set_mat4(ru.shader, "u_proj", &(renderer->proj));

    Entity e = core->generate_entity();
    e.mask = (1 << (u32)ComponentType::ParticleData) | (1 << (u32)ComponentType::ParticleRender);
    core->particle_data_comps.register_component(e, source);
    core->particle_render_comps.register_component(e, ru);
    core->entities.add(e);

    return 0;
}

#include "world.h"
#include "game.h"
#pragma warning(pop)

#pragma warning(disable : 5045) // Spectre thing

int main(void) {
    TagMap<usize> tagmap(30);
    String key("test");
    tagmap.add_or_update(&key, 3);
    usize *result = tagmap.get(&key);
    // Test more adds with collisions, and removal
    printf("result: %zd\n", *result);
    return 0;
}

int _main(void) {
    main_game();
    return 0;
}
