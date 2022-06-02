#include "util.h"

#define CHAR_COUNT 96
#define FONT_ATLAS_WIDTH 512
#define FONT_ATLAS_HEIGHT 256
#define FONT_TEXT_HEIGHT 50 // In pixels

struct TextBufferData {
    f32 *vb_data;
    usize vb_len;
    u32 *ib_data;
    usize ib_len;
};

enum class TextWidthType
{
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

    explicit FontData(const char *ttf_path) {
        u8 *font_bytes = (u8 *)read_file(ttf_path);
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

    FontData(FontData &&rhs) : ascent(rhs.ascent), descent(rhs.descent) {
        font_char_data = std::move(rhs.font_char_data);
        font_bitmap = rhs.font_bitmap;
        rhs.font_bitmap = nullptr;
    }

    FontData(const FontData &) = delete;
    FontData &operator=(const FontData &) = delete;

    ~FontData() {
        delete font_bitmap;
    }
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

    void set_str(u32 integer) {
        char int_str_buffer[32];
        sprintf_s(int_str_buffer, sizeof(char) * 32, "%d", integer);
        text = int_str_buffer;
    }
};

struct GoRenderUnit {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    u32 index_count;
    usize vert_data_len;
    shader_handle_t shader;
    texture_handle_t texture;

    GoRenderUnit(const GoRenderUnit &) = default;
    GoRenderUnit &operator=(const GoRenderUnit &) = delete;
    GoRenderUnit &operator=(GoRenderUnit &&) = delete;

    explicit GoRenderUnit(const f32 *vert_data, usize vert_data_len, const u32 *index_data,
                          usize index_data_len, shader_handle_t shader, const char *texture_file_name)
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
        uint8_t *data = stbi_load(texture_file_name, &width, &height, &channel_count, 0);
        assert(data != NULL);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data);
    }

    GoRenderUnit(GoRenderUnit &&rhs)
        : vao(rhs.vao), vbo(rhs.vbo), ibo(rhs.ibo), shader(rhs.shader), texture(rhs.texture) {
        rhs.vao = 0;
        rhs.vbo = 0;
        rhs.ibo = 0;
        rhs.shader = 0;
        rhs.texture = 0;
    }

    ~GoRenderUnit() {
        glDeleteVertexArrays(1, &(vao));
        glDeleteBuffers(1, &(vbo));
        glDeleteBuffers(1, &(ibo));
        glDeleteTextures(1, &texture);

        /* glDeleteProgram(shader); */
        // Not deleting the shader here, since we only have one instance for
        // the world. NOTE @FUTURE: Probably gonna have a batch sort of thing,
        // the guys who share the same shader
    }

    void draw(const Mat4 &model) {
        glBindVertexArray(vao);
        glBindTexture(GL_TEXTURE_2D, texture);
        shader_set_mat4(shader, "u_model", &model);
        // TODO @DOCS: How can that last parameter be zero?
        glDrawElements(GL_TRIANGLES, (GLsizei)index_count, GL_UNSIGNED_INT, 0);
    }
};

struct WidgetRenderUnit {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    u32 index_count;
    shader_handle_t shader;
    texture_handle_t texture;

    WidgetRenderUnit(const WidgetRenderUnit &rhs) = default;
    WidgetRenderUnit &operator=(const WidgetRenderUnit &rhs) = default;
    WidgetRenderUnit &operator=(WidgetRenderUnit &&rhs) = default;

    explicit WidgetRenderUnit(shader_handle_t shader, const WidgetData &widget) : shader(shader) {
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_RED,
                     GL_UNSIGNED_BYTE, widget.font_data.font_bitmap);

        // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO @ROBUSTNESS: We might
        // need to do this if we get segfaults

        glUseProgram(shader);
        shader_set_int(shader, "u_texture_ui", 0);

        update(widget);
    }

    WidgetRenderUnit(WidgetRenderUnit &&rhs)
        : vao(rhs.vao), vbo(rhs.vbo), ibo(rhs.ibo), index_count(rhs.index_count), shader(rhs.shader),
          texture(rhs.texture) {
        rhs.vao = 0;
        rhs.vbo = 0;
        rhs.ibo = 0;
        rhs.shader = 0;
        rhs.texture = 0;
    }

    ~WidgetRenderUnit() {
        glDeleteVertexArrays(1, &(vao));
        glDeleteBuffers(1, &(vbo));
        glDeleteBuffers(1, &(ibo));

        // TODO @CLEANUP: A better way to manage these shaders
        // glDeleteProgram(shader);
        glDeleteTextures(1, &texture);
    }

    void text_buffer_fill(TextBufferData *text_data, const FontData &font_data, const char *text,
                          TextTransform transform) {
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

    void update(const WidgetData &widget) {
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
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)text_data.ib_len, text_data.ib_data,
                     GL_STATIC_DRAW);
        index_count = (u32)text_data.ib_len;

        free(text_data.vb_data);
        free(text_data.ib_data);
    }

    void draw() {
        glBindVertexArray(vao);

        // TODO @ROBUSTNESS: This shouldn't be needed
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
        // start from here:
        // - C++'ize "shader_set_blah()" calls
        // - debug this "no index buffer bound" issue. hint: it goes away when only the splash ui entity is
        // registered

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glUseProgram(shader);
        glDrawElements(GL_TRIANGLES, (GLsizei)index_count, GL_UNSIGNED_INT, 0);
    }
};

struct RenderInfo {
    Mat4 view;
    Mat4 proj;
    f32 aspect;
};

static RenderInfo render_info_new(u32 screen_width, u32 screen_height, f32 cam_size) {
    RenderInfo r;

    // We translate this matrix by the cam position
    r.view = Mat4::identity();
    r.aspect = (f32)screen_width / (f32)screen_height;
    r.proj = Mat4::ortho(-r.aspect * cam_size, r.aspect * cam_size, -cam_size, cam_size, -0.001f, 100.0f);
    return r;
}
