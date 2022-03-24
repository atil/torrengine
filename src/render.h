#include "util.h"

typedef uint32_t shader_handle_t;
typedef uint32_t buffer_handle_t;
typedef uint32_t texture_handle_t;

typedef struct
{
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    uint32_t index_count;
    shader_handle_t shader;
    texture_handle_t texture;
} RenderUnit;

typedef struct
{
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    uint32_t index_count;
    shader_handle_t shader;
    texture_handle_t texture;
} UiRenderUnit;

static shader_handle_t load_shader(const char *file_path)
{
    char info_log[512]; // TODO @CLEANUP: Better logging
    char *shader_string = read_file(file_path);

    const char *vert_shader_header = "#version 420\n#define VERTEX\n";
    char *vert_string = (char *)calloc((strlen(vert_shader_header) + strlen(shader_string)), sizeof(char));
    strcat(vert_string, vert_shader_header);
    strcat(vert_string, shader_string);

    const char *frag_shader_header = "#version 420\n#define FRAGMENT\n";
    char *frag_string = (char *)calloc((strlen(frag_shader_header) + strlen(shader_string)), sizeof(char));
    strcat(frag_string, frag_shader_header);
    strcat(frag_string, shader_string);

    // TODO @LEAK: Do we need to delete these strings?

    uint32_t vertex_shader_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_handle, 1, (const char *const *)&vert_string, NULL);
    glCompileShader(vertex_shader_handle);
    int32_t success;
    glGetShaderiv(vertex_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader_handle, 512, NULL, info_log);
        printf("vertex shader fail %s\n", info_log);
    }

    uint32_t frag_shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader_handle, 1, (const char *const *)&frag_string, NULL);
    glCompileShader(frag_shader_handle);
    glGetShaderiv(frag_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(frag_shader_handle, 512, NULL, info_log);
        printf("frag shader fail %s\n", info_log);
    }

    shader_handle_t shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader_handle);
    glAttachShader(shader_program, frag_shader_handle);
    glLinkProgram(shader_program);
    glGetProgramiv(shader_program, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shader_program, 512, NULL, info_log);
        printf("shader link fail %s\n", info_log);
    }
    glDeleteShader(vertex_shader_handle);
    glDeleteShader(frag_shader_handle);

    return shader_program;
}

static void shader_set_mat4(shader_handle_t shader, const char *uniform_name, const Mat4 *mat)
{
    int32_t loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform matrix: %s\n", uniform_name);
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat->data);
}

static void shader_set_float3(shader_handle_t shader, const char *uniform_name, float f0, float f1, float f2)
{
    int32_t loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform float3: %s\n", uniform_name);
        return;
    }
    glUniform3f(loc, f0, f1, f2);
}

static void shader_set_int(shader_handle_t shader, const char *uniform_name, int i)
{
    int32_t loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform int: %s\n", uniform_name);
        return;
    }
    glUniform1i(loc, i);
}

// TODO @CLEANUP: This isn't like a constructor; it takes an existing instance and (re)initializes it. Should it be like
// a constructor and return an instance instead of taking one as a parameter?

// NOTE @FUTURE: We might think about different kinds of RenderUnits, like SolidColorRenderUnit, TextureRenderUnit,
// UiRenderUnit etc. They might have different data layout

static void render_unit_init(RenderUnit *ru, const float *vert_data, size_t vert_data_len, const uint32_t *index_data,
                             size_t index_data_len, shader_handle_t shader, const char *texture_file_name)
{
    glGenVertexArrays(1, &(ru->vao));
    glGenBuffers(1, &(ru->vbo));
    glGenBuffers(1, &(ru->ibo));

    glBindVertexArray(ru->vao);

    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferData(GL_ARRAY_BUFFER, vert_data_len, vert_data, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, index_data_len, index_data, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glGenTextures(1, &(ru->texture));
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    uint8_t *data = stbi_load(texture_file_name, &width, &height, &nrChannels, 0);
    assert(data != NULL);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);

    ru->index_count = (uint32_t)index_data_len;
    ru->shader = shader;
}

static void render_unit_deinit(RenderUnit *ru)
{
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->ibo));
    glDeleteTextures(1, &ru->texture);

    /* glDeleteProgram(ru->shader); */
    // Not deleting the shader here, since we only have one instance for the world.
    // NOTE @FUTURE: Probably gonna have a batch sort of thing, the guys who share the same shader
}

static void render_unit_ui_alloc(UiRenderUnit *ru, shader_handle_t shader, FontData *font_data)
{
    glGenVertexArrays(1, &(ru->vao));
    glGenBuffers(1, &(ru->vbo));
    glGenBuffers(1, &(ru->ibo));

    glBindVertexArray(ru->vao);

    // @DOCS: We provide empty buffers here, otherwise the pointers don't know what buffer they point to (or something)
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ru->shader = shader;

    glGenTextures(1, &(ru->texture));
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TODO @ROBUSTNESS: This depth component looks weird. Googling haven't showed up such a thing
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE,
                 font_data->font_bitmap);

    // glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // TODO @ROBUSTNESS: We might need to do this if we get segfaults

    glUseProgram(shader);
    shader_set_int(shader, "u_texture_ui", 0);
}
static void render_unit_ui_deinit(UiRenderUnit *ru)
{
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->ibo));

    // TODO @CLEANUP: A better way to manage these shaders
    // glDeleteProgram(ru->shader);
    glDeleteTextures(1, &ru->texture);
}

static void text_buffer_fill(TextBufferData *text_data, FontData *font_data, const char *text, TextTransform transform)
{
    const size_t char_count = strlen(text);

    const float x_scale = 1.0f / char_count; // TODO @DOCS: Explain

    Vec2 anchor = transform.anchor;
    Vec2 scale = transform.scale;

    uint32_t vert_curr = 0;
    uint32_t ind_curr = 0;
    for (size_t i = 0; i < char_count; i++)
    {
        char ch = text[i];
        float pixel_pos_x, pixel_pos_y;
        stbtt_aligned_quad quad;
        stbtt_GetBakedQuad(font_data->font_char_data, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, ch - ' ', &pixel_pos_x,
                           &pixel_pos_y, &quad, 1);

        // This calculation is difficult to wrap the head around. Draw it on paper to make it clearer in the head
        // descent is negative: below the baseline and the origin is the bottom
        // quadY0 is positive: it's above the baseline and the origin is top
        // quadY1 positive: below the baseline and the origin is top
        const float glyph_bottom = (-font_data->descent - quad.y1) / FONT_TEXT_HEIGHT; // In screen space
        const float glyph_top = (-quad.y0 + (-font_data->descent)) / FONT_TEXT_HEIGHT; // In screen space

        // Anchor: bottom-left corner's normalized position
        // Our quads have their origin at bottom left. But textures have their at top left. Therefore we invert the V
        // coordinate of the UVs

        // Bottom left vertex
        text_data->vb_data[vert_curr + 0] = (float)i * scale.x * x_scale + anchor.x; // X:0
        text_data->vb_data[vert_curr + 1] = glyph_bottom * scale.y + anchor.y;       // Y:0
        text_data->vb_data[vert_curr + 2] = quad.s0;                                 // U
        text_data->vb_data[vert_curr + 3] = 1.0f - quad.t1;                          // V

        // Bottom right vertex
        text_data->vb_data[vert_curr + 4] = (float)(i + 1) * scale.x * x_scale + anchor.x; // 1
        text_data->vb_data[vert_curr + 5] = glyph_bottom * scale.y + anchor.y;             // 0
        text_data->vb_data[vert_curr + 6] = quad.s1;                                       // U
        text_data->vb_data[vert_curr + 7] = 1.0f - quad.t1;                                // V

        // Top right vertex
        text_data->vb_data[vert_curr + 8] = (float)(i + 1) * scale.x * x_scale + anchor.x; // 1
        text_data->vb_data[vert_curr + 9] = glyph_top * scale.y + anchor.y;                // 1
        text_data->vb_data[vert_curr + 10] = quad.s1;                                      // U
        text_data->vb_data[vert_curr + 11] = 1.0f - quad.t0;                               // V

        // Top left vertex
        text_data->vb_data[vert_curr + 12] = (float)i * scale.x * x_scale + anchor.x; // 0
        text_data->vb_data[vert_curr + 13] = glyph_top * scale.y + anchor.y;          // 1
        text_data->vb_data[vert_curr + 14] = quad.s0;                                 // U
        text_data->vb_data[vert_curr + 15] = 1.0f - quad.t0;                          // V

        // Two triangles. Each char is 4 vertex
        text_data->ib_data[ind_curr + 0] = ((uint32_t)i * 4) + 0;
        text_data->ib_data[ind_curr + 1] = ((uint32_t)i * 4) + 1;
        text_data->ib_data[ind_curr + 2] = ((uint32_t)i * 4) + 2;
        text_data->ib_data[ind_curr + 3] = ((uint32_t)i * 4) + 0;
        text_data->ib_data[ind_curr + 4] = ((uint32_t)i * 4) + 2;
        text_data->ib_data[ind_curr + 5] = ((uint32_t)i * 4) + 3;

        vert_curr += 16;
        ind_curr += 6;
    }
}

static void render_unit_ui_update(UiRenderUnit *ru, FontData *font_data, const char *text, TextTransform transform)
{
    const size_t char_count = strlen(text);

    TextBufferData text_data;
    text_data.vb_len = char_count * 16 * sizeof(float); // TODO @DOCS: Explain the data layout
    text_data.ib_len = char_count * 6 * sizeof(uint32_t);
    text_data.vb_data = (float *)malloc(text_data.vb_len);
    text_data.ib_data = (uint32_t *)malloc(text_data.ib_len);

    text_buffer_fill(&text_data, font_data, text, transform);

    glBindVertexArray(ru->vao);
    glBindBuffer(GL_ARRAY_BUFFER, ru->vbo);
    glBufferData(GL_ARRAY_BUFFER, text_data.vb_len, text_data.vb_data, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ru->ibo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, text_data.ib_len, text_data.ib_data, GL_STATIC_DRAW);
    ru->index_count = (uint32_t)text_data.ib_len;

    free(text_data.vb_data);
    free(text_data.ib_data);
}
