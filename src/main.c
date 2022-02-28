#define GLEW_STATIC                    // Statically linking glew
#define GLFW_DLL                       // Dynamically linking glfw
#define GLFW_INCLUDE_NONE              // Disable including dev environment header
#define STB_TRUETYPE_IMPLEMENTATION    // stb requires these
#define STB_IMAGE_WRITE_IMPLEMENTATION // TODO @CLEANUP: Only needed for font debug
#define STB_IMAGE_IMPLEMENTATION

// Disable a bunch of warnings from system headers
#pragma warning(push, 0)
#pragma warning(disable : 5040) // These require extra attention for some reason
#pragma warning(disable : 5045) // Spectre thing
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <stb_image_write.h> // TODO @CLEANUP: Only needed for font debug
#include <stb_truetype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable : 4996) // TODO @ROBUSTNESS: Address these deprecated CRT functions
#pragma warning(disable : 5045) // Spectre thing
#include "util.h"
#include "tomath.h"
#include "text.h"
#include "render.h"
#pragma warning(pop)

#define WIDTH 640
#define HEIGHT 480

//
// Game objects
//

typedef struct
{
    Vec2 min;
    Vec2 max;
} Rect;

static Rect rect_new(Vec2 center, Vec2 size)
{
    float x_min = center.x - size.x / 2.0f;
    float x_max = center.x + size.x / 2.0f;
    float y_min = center.y - size.y / 2.0f;
    float y_max = center.y + size.y / 2.0f;

    Rect r;
    r.min = vec2_new(x_min, y_min);
    r.max = vec2_new(x_max, y_max);
    return r;
}

typedef struct
{
    Rect rect;
    Mat4 transform;
} GameObject;

static GameObject gameobject_new(Vec2 pos, Vec2 size)
{
    GameObject go;
    go.rect = rect_new(vec2_zero(), size);
    Mat4 transform = mat4_identity();
    mat4_translate_xy(&transform, pos);
    mat4_set_scale_xy(&transform, size);
    go.transform = transform;

    return go;
}

static Rect gameobject_get_world_rect(const GameObject *go)
{
    Vec2 go_pos = mat4_get_pos_xy(&(go->transform));
    Rect rect_world = go->rect;
    rect_world.min = vec2_add(rect_world.min, go_pos);
    rect_world.max = vec2_add(rect_world.max, go_pos);

    return rect_world;
}

static bool gameobject_is_point_in(const GameObject *go, Vec2 p)
{
    Rect rect_world = gameobject_get_world_rect(go);
    return p.x > rect_world.min.x && p.x < rect_world.max.x && p.y > rect_world.min.y && p.y < rect_world.max.y;
}

int main(void)
{
    srand((unsigned long)time(NULL));

    glfwInit();

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "torrengine.", NULL, NULL);
    glfwMakeContextCurrent(window);

    glewInit(); // Needs to be after context creation

    text_init();

    //
    // GameObject rendering
    //

    RenderUnit pad1_ru;
    RenderUnit pad2_ru;
    RenderUnit ball_ru;

    shader_handle_t world_shader = load_shader("src/world.glsl");

    // Holds position (vec3) and UV (vec2)
    float unit_square_verts[] = {-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
                                 0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f};
    uint32_t unit_square_indices[] = {0, 1, 2, 0, 2, 3};

    render_unit_init(&pad1_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader);
    render_unit_init(&pad2_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader);
    render_unit_init(&ball_ru, unit_square_verts, sizeof(unit_square_verts), unit_square_indices,
                     sizeof(unit_square_indices), world_shader);

    //
    // UI
    //

    float *text_vb;
    uint32_t *text_ib;
    size_t text_vb_len, text_ib_len;
    Vec2 text_pos = vec2_new(0.0f, 0);
    float text_scale = 1.0f;
    fill_vb_for_text("tabi", text_pos, text_scale, &text_vb, &text_vb_len, &text_ib, &text_ib_len);

    UiRenderUnit ui_ru;
    shader_handle_t ui_shader = load_shader("src/ui.glsl");
    render_unit_ui_init(&ui_ru, text_vb, text_vb_len, text_ib, text_ib_len, ui_shader, font_bitmap);

    glUseProgram(ui_ru.shader);
    shader_set_int(ui_ru.shader, "u_texture_ui", 0);

    // Loading an image:
    /* int width, height, nrChannels; */
    /* stbi_set_flip_vertically_on_load(true); */
    /* unsigned char *data = stbi_load("assets/Grass.jpg", &width, &height, &nrChannels, 0); */
    /* if (!data) */
    /* { */
    /*     printf("texture load fail\n"); */
    /* } */
    /* glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data); */
    /* glGenerateMipmap(GL_TEXTURE_2D); */
    /* stbi_image_free(data); */

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

    //
    // Game objects
    //

    const float area_half_height = cam_size;

    const float distance_from_center = 4.0f;
    Vec2 pad_size = vec2_new(0.3f, 2.0f);
    GameObject pad1_go = gameobject_new(vec2_new(distance_from_center, 0.0f), pad_size);
    GameObject pad2_go = gameobject_new(vec2_new(-distance_from_center, 0.0f), pad_size);
    GameObject ball_go = gameobject_new(vec2_zero(), vec2_scale(vec2_one(), 0.2f));

    Vec2 ball_move_dir = vec2_new(1.0f, 0.0f);
    const float ball_speed = 4.0f;

    float game_time = (float)glfwGetTime();
    float dt = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        dt = (float)glfwGetTime() - game_time;
        game_time = (float)glfwGetTime();

        //
        // Input
        //

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, true);
        }

        const float pad_move_speed = 10.0f;
        Rect pad1_world_rect = gameobject_get_world_rect(&pad1_go);
        Rect pad2_world_rect = gameobject_get_world_rect(&pad2_go);

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS && pad2_world_rect.max.y < area_half_height)
        {
            mat4_translate_xy(&pad2_go.transform, vec2_new(0.0f, pad_move_speed * dt));
        }
        else if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && pad2_world_rect.min.y > -area_half_height)
        {
            mat4_translate_xy(&pad2_go.transform, vec2_new(0.0f, -pad_move_speed * dt));
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS && pad1_world_rect.max.y < area_half_height)
        {
            mat4_translate_xy(&pad1_go.transform, vec2_new(0.0f, pad_move_speed * dt));
        }
        else if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS && pad1_world_rect.min.y > -area_half_height)
        {
            mat4_translate_xy(&pad1_go.transform, vec2_new(0.0f, -pad_move_speed * dt));
        }

        //
        // Ball move
        //

        Vec2 ball_pos = mat4_get_pos_xy(&ball_go.transform);
        Vec2 ball_displacement = vec2_scale(ball_move_dir, (ball_speed * dt));
        Vec2 ball_next_pos = vec2_add(ball_pos, ball_displacement);

        if (gameobject_is_point_in(&pad1_go, ball_next_pos) || gameobject_is_point_in(&pad2_go, ball_next_pos))
        {
            // Hit paddles
            ball_move_dir.x *= -1;
            const float ball_pad_hit_randomness_coeff = 0.2f;
            ball_move_dir.y += rand_range(1.0f, 1.0f) * ball_pad_hit_randomness_coeff;
            vec2_normalize(&ball_move_dir);

            ball_displacement = vec2_scale(ball_move_dir, (ball_speed * dt));
            ball_next_pos = vec2_add(ball_pos, ball_displacement);
        }

        if (ball_next_pos.y > area_half_height || ball_next_pos.y < -area_half_height)
        {
            // Reflection from top/bottom
            ball_move_dir.y *= -1;

            ball_displacement = vec2_scale(ball_move_dir, (ball_speed * dt));
            ball_next_pos = vec2_add(ball_pos, ball_displacement);
        }

        mat4_set_pos_xy(&ball_go.transform, ball_next_pos);

        //
        // Render
        //

        glClearColor(0.075f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /* glUseProgram(world_shader); */
        /* glBindVertexArray(pad1_ru.vao); */
        /* shader_set_mat4(world_shader, "u_model", &pad1_go.transform); */
        /* shader_set_float3(world_shader, "u_rectcolor", 1, 1, 0); */
        /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */

        /* glBindVertexArray(pad1_ru.vao); */
        /* shader_set_mat4(world_shader, "u_model", &pad2_go.transform); */
        /* shader_set_float3(world_shader, "u_rectcolor", 0, 1, 1); */
        /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */

        /* glBindVertexArray(ball_ru.vao); */
        /* shader_set_mat4(world_shader, "u_model", &ball_go.transform); */
        /* shader_set_float3(world_shader, "u_rectcolor", 1, 0, 1); */
        /* glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); */

        // UI
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, ui_ru.texture);
        glUseProgram(ui_ru.shader);
        glBindVertexArray(ui_ru.vao);

        // start from here:
        // - need that 4 to be the length of the text
        // - need to normalize all letters' heights somehow
        //   - minddump: we need to have the height of a glyph as the reference height. like for example "|" glyph goes
        //   from 0.3 to 0.5, then that means those numbers are gonna be 0 and 1 in the Y-axis in the text_vb. it
        //   actually doens't have to be the glyph, but we need a reference height

        glDrawElements(GL_TRIANGLES, 6 * 4, GL_UNSIGNED_INT, 0);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    text_deinit();

    render_unit_deinit(&pad1_ru);
    render_unit_deinit(&pad2_ru);
    render_unit_deinit(&ball_ru);
    /* render_unit_deinit(&ui_ru); */ // TODO @LEAK: this needs its own function

    glDeleteProgram(world_shader);

    glfwTerminate();
    return 0;
}
