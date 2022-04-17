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

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#include "util.h"
#include "tomath.h"
#include "particle.h"
#include "text.h"
#include "render.h"
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
    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    FontData font_data;
    text_init(&font_data);

    Sfx sfx;
    sfx_init(&sfx);

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
    //

    ParticleProps particle_props; // Example particle
    particle_props.emit_point = vec2_new(1, 2);
    particle_props.angle_limits = vec2_new(90, 270);
    particle_props.count = 5;
    particle_props.lifetime = 2;
    particle_props.speed = 1;
    particle_props.angle_offset = 0;
    particle_props.speed_offset = 0;

    ParticleSystem particle_system;
    particle_init(&particle_system, particle_props);

    ParticleRenderUnit particle_ru;
    shader_handle_t particle_shader = load_shader("src/world.glsl"); // Using world shader for now
    render_unit_particle_init(&particle_ru, particle_system.props.count, particle_shader, "assets/Ball.png");

    //
    // View-projection matrices
    //

    // We translate this matrix by the cam position
    Mat4 view = mat4_identity();
    const float cam_size = 5.0f;
    const float aspect = (float)WIDTH / (float)HEIGHT;
    Mat4 proj = mat4_ortho(-aspect * cam_size, aspect * cam_size, -cam_size, cam_size, -0.001f, 100.0f);

    glUseProgram(world_shader);
    shader_set_mat4(world_shader, "u_view", &view);
    shader_set_mat4(world_shader, "u_proj", &proj);

    glUseProgram(particle_shader);
    Mat4 mat_identity = mat4_identity();
    shader_set_mat4(particle_shader, "u_model", &mat_identity);
    shader_set_mat4(world_shader, "u_view", &view);
    shader_set_mat4(world_shader, "u_proj", &proj);

    //
    // Game objects
    //

    GameState game_state = Splash;
    PongGameConfig config;
    config.area_extents = vec2_new(cam_size * aspect, cam_size);
    config.pad_size = vec2_new(0.3f, 2.0f);
    config.ball_speed = 4.0f * 0.0000001f;
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

        // TODO @CLEANUP: This looks bad. Ideally we return this from the update function
        PongGameUpdateResult result;
        result.is_game_over = false;
        result.did_score = false;

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (game_state == Splash)
        {
            glBindVertexArray(ui_ru_splash_title.vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ui_ru_score.texture);
            glUseProgram(ui_ru_score.shader);
            glDrawElements(GL_TRIANGLES, ui_ru_splash_title.index_count, GL_UNSIGNED_INT, 0);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
                game_init(&game, &config, &sfx);
                game_state = Game;
            }
        }
        else if (game_state == Game)
        {
            result = game_update(dt, &game, &config, window, &sfx);
            if (result.is_game_over)
            {
                sfx_play(&sfx, SfxGameOver);

                game_state = GameOver;
            }

            // Game draw
            glActiveTexture(GL_TEXTURE0);
            glUseProgram(world_shader);

            glBindVertexArray(field_ru.vao);
            glBindTexture(GL_TEXTURE_2D, field_ru.texture);
            shader_set_mat4(world_shader, "u_model", &game.field_go.transform);
            glDrawElements(GL_TRIANGLES, field_ru.index_count, GL_UNSIGNED_INT, 0);

            glBindVertexArray(pad1_ru.vao);
            glBindTexture(GL_TEXTURE_2D, pad1_ru.texture);
            shader_set_mat4(world_shader, "u_model", &game.pad1_go.transform);
            glDrawElements(GL_TRIANGLES, pad1_ru.index_count, GL_UNSIGNED_INT, 0);

            glBindVertexArray(pad2_ru.vao);
            glBindTexture(GL_TEXTURE_2D, pad2_ru.texture);
            shader_set_mat4(world_shader, "u_model", &game.pad2_go.transform);
            glDrawElements(GL_TRIANGLES, pad2_ru.index_count, GL_UNSIGNED_INT, 0);

            glBindVertexArray(ball_ru.vao);
            glBindTexture(GL_TEXTURE_2D, ball_ru.texture);
            shader_set_mat4(world_shader, "u_model", &game.ball_go.transform);
            // TODO @DOCS: How can that last parameter be zero?
            glDrawElements(GL_TRIANGLES, ball_ru.index_count, GL_UNSIGNED_INT, 0);

            glBindVertexArray(pad2_ru.vao);
            glBindTexture(GL_TEXTURE_2D, pad2_ru.texture);
            shader_set_mat4(world_shader, "u_model", &game.pad2_go.transform);
            glDrawElements(GL_TRIANGLES, pad2_ru.index_count, GL_UNSIGNED_INT, 0);

            if (particle_system.isAlive)
            {
                particle_update(&particle_system, dt);

                glUseProgram(particle_shader);
                glBindVertexArray(particle_ru.vao);
                render_unit_particle_update(&particle_ru, &particle_system);
                glBindTexture(GL_TEXTURE_2D, particle_ru.texture);
                shader_set_float(particle_shader, "u_alpha", particle_system.transparency);
                glDrawElements(GL_TRIANGLES, particle_ru.index_count, GL_UNSIGNED_INT, 0);
            }

            // UI draw
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ui_ru_score.texture);
            glUseProgram(ui_ru_score.shader);
            glBindVertexArray(ui_ru_score.vao);
            if (result.did_score) // Update score view
            {
                char int_str_buffer[32]; // TODO @ROBUSTNESS: Assert that it's a 32-bit integer
                sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", game.score);
                render_unit_ui_update(&ui_ru_score, &font_data, int_str_buffer, text_transform_score);
            }
            glDrawElements(GL_TRIANGLES, ui_ru_score.index_count, GL_UNSIGNED_INT, 0);
        }
        else if (game_state == GameOver)
        {
            glBindVertexArray(ui_ru_intermission.vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, ui_ru_score.texture);
            glUseProgram(ui_ru_score.shader);
            glDrawElements(GL_TRIANGLES, ui_ru_intermission.index_count, GL_UNSIGNED_INT, 0);

            if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS)
            {
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
    render_unit_particle_deinit(&particle_ru);
    glDeleteProgram(world_shader); // TODO @CLEANUP: We'll have some sort of batching probably
    glDeleteProgram(particle_shader);

    render_unit_ui_deinit(&ui_ru_score);
    render_unit_ui_deinit(&ui_ru_intermission);
    render_unit_ui_deinit(&ui_ru_splash_title);
    glDeleteProgram(ui_shader); // TODO @CLEANUP: Same with above

    particle_deinit(&particle_system);

    glfwTerminate();
    return 0;
}
