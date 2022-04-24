#include "util.h"

struct GoRenderUnit {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    u32 index_count;
    usize vert_data_len;
    shader_handle_t shader;
    texture_handle_t texture;
};

struct UiRenderUnit {
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    u32 index_count;
    shader_handle_t shader;
    texture_handle_t texture;
};

// TODO @CLEANUP: These above ended up being the same. Is there a reason to stay this way?

struct Renderer {
    Mat4 view;
    Mat4 proj;
    f32 aspect;
};

static Renderer render_init(u32 screen_width, u32 screen_height, f32 cam_size) {
    Renderer r;

    // We translate this matrix by the cam position
    r.view = mat4_identity();
    r.aspect = (f32)screen_width / (f32)screen_height;
    r.proj = mat4_ortho(-r.aspect * cam_size, r.aspect * cam_size, -cam_size, cam_size, -0.001f, 100.0f);
    return r;
}

static GoRenderUnit render_unit_init(const f32 *vert_data, usize vert_data_len, const u32 *index_data,
                                     usize index_data_len, shader_handle_t shader, const char *texture_file_name) {
    GoRenderUnit ru;
    glGenVertexArrays(1, &(ru.vao));
    glGenBuffers(1, &(ru.vbo));
    glGenBuffers(1, &(ru.ibo));

    glBindVertexArray(ru.vao);

    glBindBuffer(GL_ARRAY_BUFFER, ru.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)vert_data_len, vert_data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru.ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)index_data_len, index_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &(ru.texture));
    glBindTexture(GL_TEXTURE_2D, ru.texture);
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

    // TODO @CLEANUP: Actually these should be usize but for some reason that causes padding in the struct
    ru.vert_data_len = (u32)vert_data_len;
    ru.index_count = (u32)index_data_len;
    ru.shader = shader;

    return ru;
}

static void render_unit_update(GoRenderUnit *ru, f32 *new_vert_data) {
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)ru->vert_data_len, new_vert_data);
}

static void render_unit_draw(GoRenderUnit *ru, Mat4 *model) {
    glBindVertexArray(ru->vao);
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    shader_set_mat4(ru->shader, "u_model", model);
    // TODO @DOCS: How can that last parameter be zero?
    glDrawElements(GL_TRIANGLES, (GLsizei)ru->index_count, GL_UNSIGNED_INT, 0);
}

static void render_unit_deinit(GoRenderUnit *ru) {
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->ibo));
    glDeleteTextures(1, &ru->texture);

    /* glDeleteProgram(ru->shader); */
    // Not deleting the shader here, since we only have one instance for
    // the world. NOTE @FUTURE: Probably gonna have a batch sort of thing,
    // the guys who share the same shader
}

static void render_unit_ui_alloc(UiRenderUnit *ru, shader_handle_t shader, FontData *font_data) {
    glGenVertexArrays(1, &(ru->vao));
    glGenBuffers(1, &(ru->vbo));
    glGenBuffers(1, &(ru->ibo));

    glBindVertexArray(ru->vao);

    // @DOCS: We provide empty buffers here, otherwise the pointers don't
    // know what buffer they point to (or something)
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (void *)(2 * sizeof(f32)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ru->shader = shader;

    glGenTextures(1, &(ru->texture));
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TODO @ROBUSTNESS: This depth component looks weird. Googling haven't
    // showed up such a thing
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE,
                 font_data->font_bitmap);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO @ROBUSTNESS: We might
    // need to do this if we get segfaults

    glUseProgram(shader);
    shader_set_int(shader, "u_texture_ui", 0);
}
static void render_unit_ui_deinit(UiRenderUnit *ru) {
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->ibo));

    // TODO @CLEANUP: A better way to manage these shaders
    // glDeleteProgram(ru->shader);
    glDeleteTextures(1, &ru->texture);
}

static void text_buffer_fill(TextBufferData *text_data, FontData *font_data, const char *text,
                             TextTransform transform) {
    const usize char_count = strlen(text);

    Vec2 anchor = transform.anchor;
    f32 width =
        transform.width_type == TextWidthType::FixedWidth ? (transform.width / (f32)char_count) : transform.width;
    f32 height = transform.height;

    u32 vert_curr = 0;
    u32 ind_curr = 0;
    for (usize i = 0; i < char_count; i++) {
        char ch = text[i];
        f32 pixel_pos_x = 0, pixel_pos_y = 0; // Don't exactly know what these are for
        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(font_data->font_char_data, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, ch - ' ', &pixel_pos_x,
                           &pixel_pos_y, &quad, 1);

        // This calculation is difficult to wrap the head around. Draw it
        // on paper to make it clearer in the head descent is negative:
        // below the baseline and the origin is the bottom quadY0 is
        // positive: it's above the baseline and the origin is top quadY1
        // positive: below the baseline and the origin is top
        const f32 glyph_bottom = (-font_data->descent - quad.y1) / FONT_TEXT_HEIGHT; // In screen space
        const f32 glyph_top = (-quad.y0 + (-font_data->descent)) / FONT_TEXT_HEIGHT; // In screen space

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

static void render_unit_ui_update(UiRenderUnit *ru, FontData *font_data, const char *text,
                                  TextTransform transform) {
    const usize char_count = strlen(text);

    TextBufferData text_data;
    text_data.vb_len = char_count * 16 * sizeof(f32); // TODO @DOCS: Explain the data layout
    text_data.ib_len = char_count * 6 * sizeof(u32);
    text_data.vb_data = (f32 *)malloc(text_data.vb_len);
    text_data.ib_data = (u32 *)malloc(text_data.ib_len);

    text_buffer_fill(&text_data, font_data, text, transform);

    glBindVertexArray(ru->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)text_data.vb_len, text_data.vb_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)text_data.ib_len, text_data.ib_data, GL_STATIC_DRAW);
    ru->index_count = (u32)text_data.ib_len;

    free(text_data.vb_data);
    free(text_data.ib_data);
}

static void render_unit_ui_draw(UiRenderUnit *ru) {
    glBindVertexArray(ru->vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    glUseProgram(ru->shader);
    glDrawElements(GL_TRIANGLES, (GLsizei)ru->index_count, GL_UNSIGNED_INT, 0);
}
