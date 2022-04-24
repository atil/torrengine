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
#pragma warning(pop)

#define WIDTH 640
#define HEIGHT 480

typedef uint32_t buffer_handle_t;
typedef uint32_t texture_handle_t;
typedef uint32_t shader_handle_t;

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#pragma warning(disable : 4505) // Unreferenced functions
#include "container.h"
#include "util.h"
#include "tomath.h"
#include "text.h"
#include "shader.h"
#include "render.h"
#include "particle.h"
#include "sfx.h"
#include "game.h"
#pragma warning(pop)

#pragma warning(disable : 5045) // Spectre thing

typedef enum
{
    Splash,
    Game,
    GameOver
} GameState;

int main(void)
{
    // array tests
    // Array arr = arr_create(sizeof(float), 10);
    // float a = 31;
    // arr_add(&arr, &a);

    // float b = ARR_GET(float, arr, 0);

    // printf("%f\n", b);
    // arr_deinit(&arr);
    // return;

    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    FontData font_data;
    text_init(&font_data);

    Sfx sfx;
    sfx_init(&sfx);

    const float cam_size = 5.0f;

    //
    // GameObject rendering
    //

    RenderUnit field_ru;
    RenderUnit pad1_ru;
    RenderUnit pad2_ru;
    RenderUnit ball_ru;

    shader_handle_t world_shader = load_shader("src/world.glsl");

    float unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.5f,  -0.5f, 1.0f, 0.0f,
                                 0.5f,  0.5f,  1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 1.0f};
    uint32_t unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    render_unit_init(&field_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader, "assets/Field.png");
    render_unit_init(&pad1_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader, "assets/PadBlue.png");
    render_unit_init(&pad2_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader, "assets/PadGreen.png");
    render_unit_init(&ball_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader, "assets/Ball.png");

    //
    // UI
    //

    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    shader_handle_t ui_shader = load_shader("src/ui.glsl");

    UiRenderUnit ui_ru_splash_title;
    render_unit_ui_alloc(&ui_ru_splash_title, ui_shader, &font_data);
    TextTransform text_transform_splash_title = texttransform_new(vec2_new(-0.8f, 0), 0.5f, FixedWidth, 1.6f);
    render_unit_ui_update(&ui_ru_splash_title, &font_data, "TorrPong!", text_transform_splash_title);

    UiRenderUnit ui_ru_score;
    render_unit_ui_alloc(&ui_ru_score, ui_shader, &font_data);
    TextTransform text_transform_score = texttransform_new(vec2_new(-0.9f, -0.9f), 0.3f, FreeWidth, 0.1f);
    // TODO @BUG: Set this with initial score
    render_unit_ui_update(&ui_ru_score, &font_data, "0", text_transform_score);

    UiRenderUnit ui_ru_intermission;
    render_unit_ui_alloc(&ui_ru_intermission, ui_shader, &font_data);
    TextTransform text_transform_intermission = texttransform_new(vec2_new(-0.75f, 0.0f), 0.5f, FixedWidth, 1.5f);
    render_unit_ui_update(&ui_ru_intermission, &font_data, "Game Over", text_transform_intermission);

    //
    // Particles

    ParticlePropRegistry particle_prop_reg = particle_prop_registry_create();
    ParticleSystemRegistry particle_system_reg = particle_system_registry_create();

    //
    // Renderer
    //

    Renderer renderer = render_init(WIDTH, HEIGHT, cam_size);

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &renderer.view);
    shader_set_mat4(world_shader, "u_proj", &renderer.proj);

    //
    // Game objects
    //

    GameState game_state = Splash;
    PongGameConfig config;
    config.area_extents = vec2_new(cam_size * renderer.aspect, cam_size);
    config.pad_size = vec2_new(0.3f, 2.0f);
    config.ball_speed = 4.0f; //* 0.0000001f;
    config.distance_from_center = 4.0f;
    config.pad_move_speed = 10.0f;
    config.game_speed_increase_coeff = 0.05f;

    PongGame game;

    float game_time = (float)glfwGetTime();
    float dt = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        dt = (float)glfwGetTime() - game_time;
        game_time = (float)glfwGetTime();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (game_state == Splash)
        {
            render_unit_ui_draw(&ui_ru_splash_title);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
                game_init(&game, &config, &sfx);
                game_state = Game;
            }
        }
        else if (game_state == Game)
        {
            PongGameUpdateResult result =
                game_update(dt, &game, &config, window, &sfx, &particle_prop_reg, &particle_system_reg, &renderer);

            if (result.is_game_over)
            {
                sfx_play(&sfx, SfxGameOver);
                game_state = GameOver;
            }

            // Game draw
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(world_shader);

            render_unit_draw(&field_ru, &game.field_go.transform);
            render_unit_draw(&pad1_ru, &game.pad1_go.transform);
            render_unit_draw(&pad2_ru, &game.pad2_go.transform);
            render_unit_draw(&ball_ru, &game.ball_go.transform);

            for (size_t i = 0; i < particle_system_reg.system_count; i++)
            {
                ParticleSystem ps = particle_system_reg.array_ptr[i];
                if (!ps.emitter->isAlive)
                {
                    particle_system_registry_remove(&particle_system_reg, ps);
                    continue;
                }

                particle_emitter_update(ps.emitter, dt);
                render_unit_particle_draw(ps.render_unit, ps.emitter);
            }

            // UI draw
            //
            if (result.did_score) // Update score view
            {
                char int_str_buffer[32]; // TODO @ROBUSTNESS: Assert that it's a 32-bit integer
                sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", game.score);
                render_unit_ui_update(&ui_ru_score, &font_data, int_str_buffer, text_transform_score);
            }

            render_unit_ui_draw(&ui_ru_score);
        }
        else if (game_state == GameOver)
        {
            render_unit_ui_draw(&ui_ru_intermission);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
                // Reset score
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, ui_ru_score.texture);
                glUseProgram(ui_ru_score.shader);
                glBindVertexArray(ui_ru_score.vao);
                render_unit_ui_update(&ui_ru_score, &font_data, "0", text_transform_score);
                glDrawElements(GL_TRIANGLES, (GLsizei)ui_ru_score.index_count, GL_UNSIGNED_INT, 0);

                game_init(&game, &config, &sfx);
                game_state = Game;
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    sfx_deinit(&sfx);

    render_unit_deinit(&field_ru);
    render_unit_deinit(&pad1_ru);
    render_unit_deinit(&pad2_ru);
    render_unit_deinit(&ball_ru);
    glDeleteProgram(world_shader); // TODO @CLEANUP: We'll have some sort of batching probably

    render_unit_ui_deinit(&ui_ru_score);
    render_unit_ui_deinit(&ui_ru_intermission);
    render_unit_ui_deinit(&ui_ru_splash_title);
    glDeleteProgram(ui_shader); // TODO @CLEANUP: Same with above

    particle_system_registry_deinit(&particle_system_reg);

    glfwTerminate();
    return 0;
}