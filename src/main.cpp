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
#include "types.h"
#include "container.h"
#include "util.h"
#include "tomath.h"
#include "core.h"
#include "text.h"
#include "shader.h"
#include "ui.h"
#include "render.h"
#include "particle.h"
#include "sfx.h"
#include "world.h"
#pragma warning(pop)

#pragma warning(disable : 5045) // Spectre thing

enum class GameState
{
    Splash,
    Game,
    GameOver
};

int main(void) {

    // start from here:
    // - put ui stuff into core
    // - consider game over case. do we recreate the core? reset it?

    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    FontData font_data;
    text_init(&font_data);

    Sfx sfx;
    sfx_init(&sfx);

    const f32 cam_size = 5.0f;

    //
    // Entities
    //

    shader_handle_t world_shader = load_shader("src/world.glsl");

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

    // TODO @INCOMPLETE: Add entity_index array. Currently there's no way to tell entities apart

    core.go_data.add(gameobject_new(vec2_zero(), vec2_new(((f32)WIDTH / (f32)HEIGHT) * 10, 10)));      // field
    core.go_data.add(gameobject_new(vec2_new(config.distance_from_center, 0.0f), config.pad_size));    // 1
    core.go_data.add(gameobject_new(vec2_new(-(config.distance_from_center), 0.0f), config.pad_size)); // 2
    core.go_data.add(gameobject_new(vec2_zero(), vec2_one() * 0.2f));                                  // ball

    core.go_render.add(render_unit_init(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                                        sizeof(unit_square_indices), world_shader, "assets/Field.png"));
    core.go_render.add(render_unit_init(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                                        sizeof(unit_square_indices), world_shader, "assets/PadBlue.png"));
    core.go_render.add(render_unit_init(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                                        sizeof(unit_square_indices), world_shader, "assets/PadGreen.png"));
    core.go_render.add(render_unit_init(unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                                        sizeof(unit_square_indices), world_shader, "assets/Ball.png"));

    //
    // UI
    //

    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_handle_t ui_shader = load_shader("src/ui.glsl");

    Widget widget_splash_title(
        "TorrPong!!\0", texttransform_new(vec2_new(-0.8f, 0), 0.5f, TextWidthType::FixedWidth, 1.6f), &font_data);
    // TODO @BUG: Set this with initial score
    Widget widget_score("0\0", texttransform_new(vec2_new(-0.9f, -0.9f), 0.3f, TextWidthType::FreeWidth, 0.1f),
                        &font_data);
    Widget widget_intermission("Game Over\0",
                               texttransform_new(vec2_new(-0.75f, 0.0f), 0.5f, TextWidthType::FixedWidth, 1.5f),
                               &font_data);

    UiRenderUnit ui_ru_splash_title;
    render_unit_ui_alloc(&ui_ru_splash_title, ui_shader, &font_data);
    render_unit_ui_update(&ui_ru_splash_title, &widget_splash_title);

    UiRenderUnit ui_ru_score;
    render_unit_ui_alloc(&ui_ru_score, ui_shader, &font_data);
    render_unit_ui_update(&ui_ru_score, &widget_score);

    UiRenderUnit ui_ru_intermission;
    render_unit_ui_alloc(&ui_ru_intermission, ui_shader, &font_data);
    render_unit_ui_update(&ui_ru_intermission, &widget_intermission);

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &renderer.view);
    shader_set_mat4(world_shader, "u_proj", &renderer.proj);

    GameState game_state = GameState::Splash;

    PongWorld world;

    f32 game_time = (f32)glfwGetTime();
    f32 dt = 0.0f;
    while (!glfwWindowShouldClose(window)) {
        dt = (f32)glfwGetTime() - game_time;
        game_time = (f32)glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (game_state == GameState::Splash) {
            render_unit_ui_draw(&ui_ru_splash_title);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                world_init(&world, &sfx);
                game_state = GameState::Game;
            }
        } else if (game_state == GameState::Game) {
            PongWorldUpdateResult result =
                world_update(dt, &world, &core, &config, window, &sfx, &particle_prop_reg, &renderer);

            if (result.is_game_over) {
                sfx_play(&sfx, SfxId::SfxGameOver);
                game_state = GameState::GameOver;
            }

            // Game draw
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(world_shader);

            for (usize i = 0; i < core.go_render.count; i++) {
                render_unit_draw(core.go_render[i], &(core.go_data[i]->transform));
            }

            // Particle update/draw
            Array<EntityIndex> dead_particle_indices(core.particle_sources.count);
            for (usize i = 0; i < core.particle_sources.count; i++) {
                ParticleSource *pe = core.particle_sources[i];
                if (!pe->isAlive) {
                    dead_particle_indices.add(i);
                    continue;
                }

                particle_emitter_update(pe, dt);
                render_unit_particle_draw(core.particle_render[i], pe);
            }
            for (EntityIndex i = 0; i < dead_particle_indices.count; i++) {
                particle_despawn(&core, i);
            }

            // UI draw
            if (result.did_score) {
                // Update score view
                widget_set_string(&widget_score, world.score);
                render_unit_ui_update(&ui_ru_score, &widget_score);
            }

            render_unit_ui_draw(&ui_ru_score);
        } else if (game_state == GameState::GameOver) {
            render_unit_ui_draw(&ui_ru_intermission);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
                // Reset score
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, ui_ru_score.texture);
                glUseProgram(ui_ru_score.shader);
                glBindVertexArray(ui_ru_score.vao);
                render_unit_ui_update(&ui_ru_score, &widget_score);
                glDrawElements(GL_TRIANGLES, (GLsizei)ui_ru_score.index_count, GL_UNSIGNED_INT, 0);
                // render_unit_ui_draw(&ui_ru_score, &widget_score);

                world_init(&world, &sfx);
                game_state = GameState::Game;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    sfx_deinit(&sfx);

    glDeleteProgram(world_shader); // TODO @CLEANUP: We'll have some sort of batching probably

    render_unit_ui_deinit(&ui_ru_score);
    render_unit_ui_deinit(&ui_ru_intermission);
    render_unit_ui_deinit(&ui_ru_splash_title);
    glDeleteProgram(ui_shader); // TODO @CLEANUP: Same with above

    glfwTerminate();
    return 0;
}
