// start from here:
// - make the engine a library. in which way do we divide things into its own modules?
// - write down which structs need ctor/dtor, i.e. the ones that can be array elements
// - implement Array::add_move(). deepcopy'ing stuff every time we add to array is bad
// - rename: deinit->dispose
// - mat4 shenanigans
// - EntityId struct. keeps EntityIndex and a tag string

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

    core->particle_sources.add(source);
    core->particle_render.add(ru);

    return core->particle_sources.count - 1;
}

static void deregister_particle(Core *core, EntityIndex ent_index) {
    ParticleSource *ps = core->particle_sources[ent_index];
    // NOTE @BUGFIX: Needs to be before the remove, since remove completely destroys the element
    particle_source_deinit(ps);
    core->particle_sources.remove(ps);
    // NOTE @BUGFIX: We don't free *ps here, because we didn't allocate that ps with malloc, but on the stack

    ParticleRenderUnit *ru = core->particle_render[ent_index];
    render_unit_particle_deinit(ru);
    core->particle_render.remove(ru);
}

#include "world.h"
#pragma warning(pop)

#pragma warning(disable : 5045) // Spectre thing

enum class GameState
{
    Splash,
    Game,
    GameOver
};

EntityIndex register_gameobject(Core *core, Vec2 pos, Vec2 size, char *texture_path,
                                shader_handle_t world_shader) {

    f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                               0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
    u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    assert(core->go_data.count == core->go_render.count);

    core->go_data.add(gameobject_new(pos, size));
    core->go_render.add(render_unit_init(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                                         sizeof(unit_square_indices), world_shader, texture_path));
    return core->go_data.count - 1;
}

EntityIndex register_ui_entity(Core *core, char *text, TextTransform transform, FontData *font_data,
                               shader_handle_t ui_shader) {
    assert(core->ui_widgets.count == core->ui_render.count);
    Widget widget(text, transform, font_data);
    core->ui_widgets.add(widget);
    core->ui_render.add(render_unit_ui_init(ui_shader, &widget));
    return core->ui_widgets.count - 1;
}

void reset_game(Core *core, PongEntities *entities, PongWorldConfig *config) {
    core->go_data.replace(gameobject_new(vec2_zero(), vec2_new(((f32)WIDTH / (f32)HEIGHT) * 10, 10)),
                          (usize)entities->entity_world_field);
    core->go_data.replace(gameobject_new(vec2_new(config->distance_from_center, 0.0f), config->pad_size),
                          (usize)entities->entity_world_pad1);
    core->go_data.replace(gameobject_new(vec2_new(-(config->distance_from_center), 0.0f), config->pad_size),
                          (usize)entities->entity_world_pad2);
    core->go_data.replace(gameobject_new(vec2_zero(), vec2_one() * 0.2f), (usize)entities->entity_world_ball);

    // Reset score
    EntityIndex entity_ui_score = entities->entity_ui_score;
    widget_set_string(core->ui_widgets[entity_ui_score], 0);
    render_unit_ui_update(core->ui_render[entity_ui_score], core->ui_widgets[entity_ui_score]);
    render_unit_ui_draw(core->ui_render[entity_ui_score]);
}

// This is gonna be a part of the game module
static void loop(Core *core, PongWorld *world, PongEntities *entities, PongWorldConfig *config, GLFWwindow *window,
                 Input *input, Renderer *renderer, ParticlePropRegistry *particle_prop_reg, Sfx *sfx,
                 shader_handle_t world_shader) {

    EntityIndex entity_ui_score = entities->entity_ui_score;

    GameState game_state = GameState::Splash;
    f32 game_time = (f32)glfwGetTime();
    f32 dt = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        dt = (f32)glfwGetTime() - game_time;
        game_time = (f32)glfwGetTime();

        input->update(window);

        if (input->just_pressed(KeyCode::Esc)) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (game_state == GameState::Splash) {
            render_unit_ui_draw(core->ui_render[entities->entity_ui_splash]);

            if (input->just_pressed(KeyCode::Enter)) {
                world_init(world);
                sfx_play(sfx, SfxId::SfxStart);

                game_state = GameState::Game;
            }
        } else if (game_state == GameState::Game) {
            PongWorldUpdateResult result =
                world_update(dt, world, core, config, entities, input, sfx, particle_prop_reg, renderer);

            if (result.is_game_over) {
                sfx_play(sfx, SfxId::SfxGameOver);
                game_state = GameState::GameOver;
            }

            // Game draw
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(world_shader);

            for (usize i = 0; i < core->go_render.count; i++) {
                render_unit_draw(core->go_render[i], &(core->go_data[i]->transform));
            }

            // Particle update/draw
            Array<EntityIndex> dead_particle_indices(core->particle_sources.count);
            for (usize i = 0; i < core->particle_sources.count; i++) {
                ParticleSource *pe = core->particle_sources[i];
                if (!pe->isAlive) {
                    dead_particle_indices.add(i);
                    continue;
                }

                particle_source_update(pe, dt);
                render_unit_particle_draw(core->particle_render[i], pe);
            }
            for (EntityIndex i = 0; i < dead_particle_indices.count; i++) {
                deregister_particle(core, i);
            }

            // UI draw
            if (result.did_score) {
                // Update score view
                widget_set_string(core->ui_widgets[entity_ui_score], world->score);
                render_unit_ui_update(core->ui_render[entity_ui_score], core->ui_widgets[entity_ui_score]);
            }

            render_unit_ui_draw(core->ui_render[entity_ui_score]);
        } else if (game_state == GameState::GameOver) {
            render_unit_ui_draw(core->ui_render[entities->entity_ui_intermission]);

            if (input->just_pressed(KeyCode::Enter)) {
                reset_game(core, entities, config);
                world_init(world);
                sfx_play(sfx, SfxId::SfxStart);
                game_state = GameState::Game;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(void) {

    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    FontData font_data;
    Sfx sfx;

    const f32 cam_size = 5.0f;

    //
    // Entities
    //

    f32 unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                               0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
    u32 unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    Renderer renderer = render_init(WIDTH, HEIGHT, cam_size);

    PongWorldConfig config;
    config.area_extents = vec2_new(cam_size * renderer.aspect, cam_size);
    config.pad_size = vec2_new(0.3f, 2.0f);
    config.ball_speed = 4.0f;
    config.distance_from_center = 4.0f;
    config.pad_move_speed = 10.0f;
    config.game_speed_increase_coeff = 0.05f;

    Core core;

    shader_handle_t world_shader = load_shader("src/world.glsl");
    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_handle_t ui_shader = load_shader("src/ui.glsl");

    PongEntities entities;
    entities.entity_ui_splash = register_ui_entity(
        &core, "TorrPong!\0", texttransform_new(vec2_new(-0.8f, 0), 0.5f, TextWidthType::FixedWidth, 1.6f),
        &font_data, ui_shader);
    entities.entity_ui_score = register_ui_entity(
        &core, "0\0", texttransform_new(vec2_new(-0.9f, -0.9f), 0.3f, TextWidthType::FreeWidth, 0.1f), &font_data,
        ui_shader);
    entities.entity_ui_intermission = register_ui_entity(
        &core, "Game Over\0", texttransform_new(vec2_new(-0.75f, 0.0f), 0.5f, TextWidthType::FixedWidth, 1.5f),
        &font_data, ui_shader);

    entities.entity_world_field = register_gameobject(
        &core, vec2_zero(), vec2_new(((f32)WIDTH / (f32)HEIGHT) * 10, 10), "assets/Field.png", world_shader);
    entities.entity_world_pad1 = register_gameobject(&core, vec2_new(config.distance_from_center, 0.0f),
                                                     config.pad_size, "assets/PadBlue.png", world_shader);
    entities.entity_world_pad2 = register_gameobject(&core, vec2_new(-config.distance_from_center, 0.0f),
                                                     config.pad_size, "assets/PadGreen.png", world_shader);
    entities.entity_world_ball =
        register_gameobject(&core, vec2_zero(), vec2_one() * 0.2f, "assets/Ball.png", world_shader);

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &renderer.view);
    shader_set_mat4(world_shader, "u_proj", &renderer.proj);

    PongWorld world;
    Input input;

    loop(&core, &world, &entities, &config, window, &input, &renderer, &particle_prop_reg, &sfx, world_shader);

    //
    // Cleanup
    //

    // TODO @CLEANUP: We'll have some sort of batching for these two
    glDeleteProgram(world_shader);
    glDeleteProgram(ui_shader);

    glfwTerminate();
    return 0;
}
