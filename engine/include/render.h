#pragma once

DISABLE_WARNINGS
#include <string>
#include <array>
#include <stb_truetype.h>
#include <stb_image.h>
ENABLE_WARNINGS

#include "util.h"
#include "tomath.h"

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256
#define FONT_TEXT_HEIGHT 50 // In pixels

struct RenderInfo {
    Mat4 view;
    Mat4 proj;
    u32 width;
    u32 height;
    f32 aspect;
    f32 cam_size;

    RenderInfo(Mat4 view, Mat4 proj, u32 screen_width, u32 screen_height, f32 cam_size)
        : view(view), proj(proj), width(screen_width), height(screen_height),
          aspect((f32)screen_width / (f32)screen_height), cam_size(cam_size) {
    }
    RenderInfo() {
    }
};

struct Renderer {
    RenderInfo render_info;
    // TODO @ROBUSTNESS: Have a Shader struct and make these unique_ptr's
    shader_handle world_shader;
    shader_handle ui_shader;

    Renderer(u32 screen_width, u32 screen_height, f32 cam_size);

    ~Renderer();
};

struct TextBufferData {
    f32 *vb_data;
    usize vb_len;
    u32 *ib_data;
    usize ib_len;
};

enum class TextWidthType {
    FixedWidth,
    FreeWidth,
};

struct TextTransform {
    Vec2 anchor;
    f32 height; // In NDC
    TextWidthType width_type;
    f32 width; // In NDC

    explicit TextTransform(Vec2 anchor, f32 height, TextWidthType width_type, f32 width)
        : anchor(anchor), height(height), width_type(width_type), width(width) {
    }
};

struct FontData {
    u8 *font_bitmap;
    f32 ascent;  // In pixels
    f32 descent; // In pixels
    std::array<stbtt_bakedchar, CHAR_COUNT> font_char_data;

    explicit FontData(const std::string &ttf_path);

    FontData(FontData &&rhs);
    FontData(const FontData &) = delete;
    FontData &operator=(const FontData &) = delete;

    ~FontData();
};

struct WidgetData {
    std::string text;
    TextTransform transform;
    u8 _padding[4];
    const FontData &font_data;

    explicit WidgetData(const std::string &text, TextTransform transform, const FontData &font_data)
        : text(text), transform(transform), font_data(font_data) {
    }

    WidgetData(const WidgetData &) = default;
    WidgetData(WidgetData &&) = default;
    WidgetData &operator=(WidgetData &&) = delete;
    WidgetData &operator=(const WidgetData &) = delete;

    void set_str(u32 integer);
};

struct GoRenderUnit {
    buffer_handle vao;
    buffer_handle vbo;
    buffer_handle ibo;
    u32 index_count;
    usize vert_data_len;
    shader_handle shader;
    texture_handle texture;

    GoRenderUnit(const GoRenderUnit &) = delete;
    GoRenderUnit &operator=(const GoRenderUnit &) = delete;
    GoRenderUnit &operator=(GoRenderUnit &&) = delete;

    explicit GoRenderUnit(const f32 *vert_data, usize vert_data_len, const u32 *index_data,
                          usize index_data_len, shader_handle shader, const std::string &texture_file_name);

    GoRenderUnit(GoRenderUnit &&rhs);
    ~GoRenderUnit();

    void draw(const Mat4 &model);
};

struct WidgetRenderUnit {
    buffer_handle vao;
    buffer_handle vbo;
    buffer_handle ibo;
    u32 index_count;
    shader_handle shader;
    texture_handle texture;

    WidgetRenderUnit(const WidgetRenderUnit &rhs) = default;
    WidgetRenderUnit &operator=(const WidgetRenderUnit &rhs) = default;
    WidgetRenderUnit &operator=(WidgetRenderUnit &&rhs) = default;

    explicit WidgetRenderUnit(shader_handle shader, const WidgetData &widget);
    WidgetRenderUnit(WidgetRenderUnit &&rhs);
    ~WidgetRenderUnit();

    void text_buffer_fill(TextBufferData *text_data, const FontData &font_data, const char *text,
                          TextTransform transform);

    void update(const WidgetData &widget);

    void draw();
};

struct ParticleRenderUnit {
    buffer_handle vao;
    buffer_handle vbo;
    buffer_handle uv_bo;
    buffer_handle ibo;
    u32 index_count;
    shader_handle shader;
    texture_handle texture;
    u32 vert_data_len; // TODO @CLEANUP: Why is this u32?
    f32 *vert_data;

    explicit ParticleRenderUnit(usize particle_count, RenderInfo render_info,
                                const std::string &texture_file_name);

    ParticleRenderUnit(ParticleRenderUnit &&rhs);
    ParticleRenderUnit &operator=(ParticleRenderUnit &&rhs) = delete;
    ParticleRenderUnit(const ParticleRenderUnit &rhs) = delete;
    ParticleRenderUnit &operator=(const ParticleRenderUnit &rhs) = delete;

    ~ParticleRenderUnit();
    void draw(const struct ParticleSource &ps);
};