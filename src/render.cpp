#define GLEW_STATIC                 // Statically linking glew
#define STB_TRUETYPE_IMPLEMENTATION // stb requires these
#define STB_IMAGE_IMPLEMENTATION

#include "common.h"

DISABLE_WARNINGS
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cassert>
#include <string>
#include <GL/glew.h>
ENABLE_WARNINGS

#include "util.h"
#include "tomath.h"
#include "render.h"
#include "shader.h"
#include "particle.h"

// TODO @ROBUSTNESS: Implement glDebugMessageCallback

void glDebugOutput(GLenum source, GLenum type, unsigned int id, GLenum severity, GLsizei length,
                   const char *message, const void *userParam) {
    if (id == 131169 || id == 131185 || id == 131218 || id == 131204)
        return;
    printf("%s\n", message);
}

Renderer::Renderer(u32 screen_width, u32 screen_height) {

    const f32 cam_size = 5.0f; // TODO @CLEANUP: Magic number looks bad
    Mat4 view = Mat4::identity();
    f32 aspect = (f32)screen_width / (f32)screen_height;
    Mat4 proj = Mat4::ortho(-aspect * cam_size, aspect * cam_size, -cam_size, cam_size, -0.001f, 100.0f);
    render_info = RenderInfo(view, proj, screen_width, screen_height);

    glewInit(); // Needs to be after GLFW init

    ui_shader = Shader::load("src/shader/ui.glsl");
    world_shader = Shader::load("src/shader/world.glsl");
    glEnable(GL_BLEND); // Enabling transparency for texts
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // OpenGL debug output
    // TODO @CLEANUP: Bind this to a switch or something
    // glEnable(GL_DEBUG_OUTPUT);
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    // glDebugMessageCallback(glDebugOutput, nullptr);
    // glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);

    glUseProgram(world_shader);
    Shader::set_mat4(world_shader, "u_view", &view);
    Shader::set_mat4(world_shader, "u_proj", &proj);
}

Renderer::~Renderer() {

    // TODO @CLEANUP: We'll have some sort of batching for these two
    glDeleteProgram(world_shader);
    glDeleteProgram(ui_shader);
}

//
// Font data
//
FontData::FontData(const std::string &ttf_path) {
    u8 *font_bytes = (u8 *)Util::read_file(ttf_path.c_str());
    assert(font_bytes != nullptr);
    font_bitmap = new u8[FONT_ATLAS_WIDTH * FONT_ATLAS_HEIGHT];

    stbtt_BakeFontBitmap((u8 *)font_bytes, 0, FONT_TEXT_HEIGHT, font_bitmap, FONT_ATLAS_WIDTH,
                         FONT_ATLAS_HEIGHT, ' ', CHAR_COUNT, font_char_data.data());

    stbtt_fontinfo font_info;
    stbtt_InitFont(&font_info, font_bytes, 0);

    int ascent_int, descent_int, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent_int, &descent_int, &line_gap);

    f32 scale = stbtt_ScaleForPixelHeight(&font_info, FONT_TEXT_HEIGHT);
    ascent = (f32)ascent_int * scale;
    descent = (f32)descent_int * scale;

    free(font_bytes);
}

FontData::FontData(FontData &&rhs) : ascent(rhs.ascent), descent(rhs.descent) {
    font_char_data = std::move(rhs.font_char_data);
    font_bitmap = rhs.font_bitmap;
    rhs.font_bitmap = nullptr;
}

FontData::~FontData() {
    delete font_bitmap;
}

void WidgetData::set_str(u32 integer) {
    char int_str_buffer[32];
    sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", integer);
    text = int_str_buffer;
}

//
// GoRenderUnit
//
GoRenderUnit::GoRenderUnit(const f32 *vert_data, usize vert_data_len, const u32 *index_data,
                           usize index_data_len, shader_handle_t shader, const std::string &texture_file_name)
    : index_count((u32)index_data_len), vert_data_len(vert_data_len), shader(shader) {

    glGenVertexArrays(1, &(vao));
    glGenBuffers(1, &(vbo));
    glGenBuffers(1, &(ibo));

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vert_data_len, vert_data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)index_data_len, index_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &(texture));
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, channel_count;
    stbi_set_flip_vertically_on_load(true);
    uint8_t *data = stbi_load(texture_file_name.c_str(), &width, &height, &channel_count, 0);
    assert(data != nullptr);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
}

GoRenderUnit::GoRenderUnit(GoRenderUnit &&rhs)
    : vao(rhs.vao), vbo(rhs.vbo), ibo(rhs.ibo), index_count(rhs.index_count),
      vert_data_len(rhs.vert_data_len), shader(rhs.shader), texture(rhs.texture) {
    rhs.vao = 0;
    rhs.vbo = 0;
    rhs.ibo = 0;
    rhs.shader = 0;
    rhs.texture = 0;
}

GoRenderUnit::~GoRenderUnit() {
    glDeleteVertexArrays(1, &(vao));
    glDeleteBuffers(1, &(vbo));
    glDeleteBuffers(1, &(ibo));
    glDeleteTextures(1, &texture);

    /* glDeleteProgram(shader); */
    // Not deleting the shader here, since we only have one instance for
    // the world. NOTE @FUTURE: Probably gonna have a batch sort of thing,
    // the guys who share the same shader
}

void GoRenderUnit::draw(const Mat4 &model) {

    glBindVertexArray(vao);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(shader);
    Shader::set_mat4(shader, "u_model", &model);
    // TODO @DOCS: How can that last parameter be zero?
    glDrawElements(GL_TRIANGLES, (GLsizei)index_count, GL_UNSIGNED_INT, 0);
}

//
// WidgetRenderUnit
//

WidgetRenderUnit::WidgetRenderUnit(shader_handle_t shader, const WidgetData &widget) : shader(shader) {
    glGenVertexArrays(1, &(vao));
    glGenBuffers(1, &(vbo));
    glGenBuffers(1, &(ibo));

    glBindVertexArray(vao);

    // @DOCS: We provide empty buffers here, otherwise the pointers don't
    // know what buffer they point to (or something)
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &(texture));
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TODO @ROBUSTNESS: This depth component looks weird. Googling haven't
    // showed up such a thing
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE,
                 widget.font_data.font_bitmap);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO @ROBUSTNESS: We might
    // need to do this if we get segfaults

    glUseProgram(shader);
    Shader::set_int(shader, "u_texture_ui", 0);

    update(widget);
}

WidgetRenderUnit::WidgetRenderUnit(WidgetRenderUnit &&rhs)
    : vao(rhs.vao), vbo(rhs.vbo), ibo(rhs.ibo), index_count(rhs.index_count), shader(rhs.shader),
      texture(rhs.texture) {
    rhs.vao = 0;
    rhs.vbo = 0;
    rhs.ibo = 0;
    rhs.shader = 0;
    rhs.texture = 0;
}

WidgetRenderUnit::~WidgetRenderUnit() {
    glDeleteVertexArrays(1, &(vao));
    glDeleteBuffers(1, &(vbo));
    glDeleteBuffers(1, &(ibo));

    // TODO @CLEANUP: A better way to manage these shaders
    // glDeleteProgram(shader);
    glDeleteTextures(1, &texture);
}

void WidgetRenderUnit::text_buffer_fill(TextBufferData *text_data, const FontData &font_data,
                                        const char *text, TextTransform transform) {
    const usize char_count = strlen(text);

    Vec2 anchor = transform.anchor;
    f32 width = transform.width_type == TextWidthType::FixedWidth ? (transform.width / (f32)char_count)
                                                                  : transform.width;
    f32 height = transform.height;

    u32 vert_curr = 0;
    u32 ind_curr = 0;
    for (usize i = 0; i < char_count; i++) {
        char ch = text[i];
        f32 pixel_pos_x = 0, pixel_pos_y = 0; // Don't exactly know what these are for
        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(font_data.font_char_data.data(), FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, ch - ' ',
                           &pixel_pos_x, &pixel_pos_y, &quad, 1);

        // This calculation is difficult to wrap the head around. Draw it
        // on paper to make it clearer in the head descent is negative:
        // below the baseline and the origin is the bottom quadY0 is
        // positive: it's above the baseline and the origin is top quadY1
        // positive: below the baseline and the origin is top
        const f32 glyph_bottom = (-font_data.descent - quad.y1) / FONT_TEXT_HEIGHT; // In screen space
        const f32 glyph_top = (-quad.y0 + (-font_data.descent)) / FONT_TEXT_HEIGHT; // In screen space

        // Anchor: bottom-left corner's normalized position
        // Our quads have their origin at bottom left. But textures have
        // their at top left. Therefore we invert the V coordinate of the
        // UVs

        // Bottom left vertex
        text_data->vb_data[vert_curr + 0] = (f32)i * width + anchor.x;        // X:0
        text_data->vb_data[vert_curr + 1] = glyph_bottom * height + anchor.y; // Y:0
        text_data->vb_data[vert_curr + 2] = quad.s0;                          // U
        text_data->vb_data[vert_curr + 3] = 1.0f - quad.t1;                   // V

        // Bottom right vertex
        text_data->vb_data[vert_curr + 4] = (f32)(i + 1) * width + anchor.x;  // 1
        text_data->vb_data[vert_curr + 5] = glyph_bottom * height + anchor.y; // 0
        text_data->vb_data[vert_curr + 6] = quad.s1;                          // U
        text_data->vb_data[vert_curr + 7] = 1.0f - quad.t1;                   // V

        // Top right vertex
        text_data->vb_data[vert_curr + 8] = (f32)(i + 1) * width + anchor.x; // 1
        text_data->vb_data[vert_curr + 9] = glyph_top * height + anchor.y;   // 1
        text_data->vb_data[vert_curr + 10] = quad.s1;                        // U
        text_data->vb_data[vert_curr + 11] = 1.0f - quad.t0;                 // V

        // Top left vertex
        text_data->vb_data[vert_curr + 12] = (f32)i * width + anchor.x;     // 0
        text_data->vb_data[vert_curr + 13] = glyph_top * height + anchor.y; // 1
        text_data->vb_data[vert_curr + 14] = quad.s0;                       // U
        text_data->vb_data[vert_curr + 15] = 1.0f - quad.t0;                // V

        // Two triangles. Each char is 4 vertex
        text_data->ib_data[ind_curr + 0] = ((u32)i * 4) + 0;
        text_data->ib_data[ind_curr + 1] = ((u32)i * 4) + 1;
        text_data->ib_data[ind_curr + 2] = ((u32)i * 4) + 2;
        text_data->ib_data[ind_curr + 3] = ((u32)i * 4) + 0;
        text_data->ib_data[ind_curr + 4] = ((u32)i * 4) + 2;
        text_data->ib_data[ind_curr + 5] = ((u32)i * 4) + 3;

        vert_curr += 16;
        ind_curr += 6;
    }
}

void WidgetRenderUnit::update(const WidgetData &widget) {
    usize char_count = widget.text.length();

    TextBufferData text_data;
    text_data.vb_len = char_count * 16 * sizeof(f32); // TODO @DOCS: Explain the data layout
    text_data.ib_len = char_count * 6 * sizeof(u32);
    text_data.vb_data = (f32 *)malloc(text_data.vb_len);
    text_data.ib_data = (u32 *)malloc(text_data.ib_len);

    text_buffer_fill(&text_data, widget.font_data, widget.text.c_str(),
                     widget.transform); // TODO @CLEANUP: Send the string itself

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)text_data.vb_len, text_data.vb_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)text_data.ib_len, text_data.ib_data, GL_STATIC_DRAW);
    index_count = (u32)text_data.ib_len;

    free(text_data.vb_data);
    free(text_data.ib_data);
}

void WidgetRenderUnit::draw() {
    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);
    glUseProgram(shader);
    glDrawElements(GL_TRIANGLES, (GLsizei)index_count, GL_UNSIGNED_INT, 0);
}

//
// ParticleRenderUnit
//

ParticleRenderUnit::ParticleRenderUnit(usize particle_count, RenderInfo render_info,
                                       const std::string &texture_file_name) {

    shader = Shader::load("src/shader/world.glsl"); // Using world shader for now

    glUseProgram(shader);
    Mat4 mat_identity = Mat4::identity();
    Shader::set_mat4(shader, "u_model", &mat_identity);
    Shader::set_mat4(shader, "u_view", &(render_info.view));
    Shader::set_mat4(shader, "u_proj", &(render_info.proj));

    // TODO @CLEANUP: VLAs would simplify this allocation
    f32 single_particle_vert[8] = {-0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f};
    vert_data_len = (u32)particle_count * sizeof(single_particle_vert);
    vert_data = (f32 *)malloc(vert_data_len);

    f32 single_particle_uvs[8] = {0, 0, 1, 0, 1, 1, 0, 1};
    usize uv_data_len = particle_count * sizeof(single_particle_uvs);
    f32 *uv_data = (f32 *)malloc(uv_data_len);

    u32 single_particle_index[6] = {0, 1, 2, 0, 2, 3};
    usize index_data_len = particle_count * sizeof(single_particle_index);
    u32 *index_data = (u32 *)malloc(index_data_len);

    for (u32 i = 0; i < (u32)particle_count; i++) {
        // Learning: '+' operator for pointers doesn't increment by bytes.
        // The increment amount is of the pointer's type. So for this one above, it increments 8 f32s.

        memcpy(vert_data + i * 8, single_particle_vert, sizeof(single_particle_vert));

        u32 particle_index_at_i[6] = {
            single_particle_index[0] + (i * 4), single_particle_index[1] + (i * 4),
            single_particle_index[2] + (i * 4), single_particle_index[3] + (i * 4),
            single_particle_index[4] + (i * 4), single_particle_index[5] + (i * 4),
        };
        memcpy(index_data + i * 6, particle_index_at_i, sizeof(particle_index_at_i));

        memcpy(uv_data + i * 8, single_particle_uvs, sizeof(single_particle_uvs));
    }

    index_count = (u32)index_data_len;

    glGenVertexArrays(1, &(vao));
    glGenBuffers(1, &(vbo));
    glGenBuffers(1, &(ibo));
    glGenBuffers(1, &(uv_bo));

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)vert_data_len, vert_data, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);

    glBindBuffer(GL_ARRAY_BUFFER, uv_bo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizei)uv_data_len, uv_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(f32), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizei)index_data_len, index_data, GL_STATIC_DRAW);

    glGenTextures(1, &(texture));
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    i32 width, height, channel_count;
    stbi_set_flip_vertically_on_load(true);
    u8 *data = stbi_load(texture_file_name.c_str(), &width, &height, &channel_count, 0);
    assert(data != NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    free(index_data);
    free(uv_data);
}

ParticleRenderUnit::ParticleRenderUnit(ParticleRenderUnit &&rhs)
    : vao(rhs.vao), vbo(rhs.vbo), uv_bo(rhs.uv_bo), ibo(rhs.ibo), index_count(rhs.index_count),
      shader(rhs.shader), texture(rhs.texture), vert_data_len(rhs.vert_data_len) {

    rhs.vao = 0;
    rhs.vbo = 0;
    rhs.uv_bo = 0;
    rhs.ibo = 0;
    rhs.shader = 0;
    rhs.texture = 0;

    vert_data = rhs.vert_data;
    rhs.vert_data = nullptr;
}

ParticleRenderUnit::~ParticleRenderUnit() {
    glDeleteVertexArrays(1, &(vao));
    glDeleteBuffers(1, &(vbo));
    glDeleteBuffers(1, &(uv_bo));
    glDeleteBuffers(1, &(ibo));

    glDeleteProgram(shader); // This shader is created for this renderUnit specifically
    glDeleteTextures(1, &texture);

    free(vert_data);
}

void ParticleRenderUnit::draw(const ParticleSource &ps) {

    glUseProgram(shader);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    f32 half_particle_size = ps.props.size * 0.5f;
    for (u32 i = 0; i < ps.props.count; i++) {
        Vec2 particle_pos = ps.positions[i];
        vert_data[(i * 8) + 0] = particle_pos.x - half_particle_size;
        vert_data[(i * 8) + 1] = particle_pos.y - half_particle_size;
        vert_data[(i * 8) + 2] = particle_pos.x + half_particle_size;
        vert_data[(i * 8) + 3] = particle_pos.y - half_particle_size;
        vert_data[(i * 8) + 4] = particle_pos.x + half_particle_size;
        vert_data[(i * 8) + 5] = particle_pos.y + half_particle_size;
        vert_data[(i * 8) + 6] = particle_pos.x - half_particle_size;
        vert_data[(i * 8) + 7] = particle_pos.y + half_particle_size;
    }

    glBufferSubData(GL_ARRAY_BUFFER, 0, vert_data_len, vert_data);

    glBindTexture(GL_TEXTURE_2D, texture);
    Shader::set_f32(shader, "u_alpha", ps.transparency);
    glDrawElements(GL_TRIANGLES, (GLsizeiptr)index_count, GL_UNSIGNED_INT, 0);
}
