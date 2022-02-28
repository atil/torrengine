#include "util.h"

typedef uint32_t shader_handle_t;
typedef uint32_t buffer_handle_t;
typedef uint32_t texture_handle_t;

typedef struct
{
    buffer_handle_t vao;
    buffer_handle_t vbo;
    buffer_handle_t ibo;
    shader_handle_t shader;
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

// TODO @CLEANUP: This isn't like a constructor; it takes an existing instance and (re)initializes it. Should it be like
// a constructor and return an instance instead of taking one as a parameter?

// NOTE @FUTURE: We might think about different kinds of RenderUnits, like SolidColorRenderUnit, TextureRenderUnit,
// UiRenderUnit etc. They might have different data layout

static void render_unit_init(RenderUnit *ru, const float *vert_data, size_t vert_data_len, const uint32_t *index_data,
                             size_t index_data_len, shader_handle_t shader)
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

    ru->shader = shader;
}

// TODO @CLEANUP: This texture_data is currently the font atlas. What would we do when we want to render both this and a
// textured UI element?
static void render_unit_ui_init(UiRenderUnit *ru, const float *vert_data, size_t vert_data_len,
                                const uint32_t *index_data, size_t index_data_len, shader_handle_t shader,
                                const uint8_t *texture_data)
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
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    ru->index_count = (uint32_t)index_data_len;
    ru->shader = shader;

    glGenTextures(1, &(ru->texture));
    glBindTexture(GL_TEXTURE_2D, ru->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // TODO @ROBUSTNESS: This depth component looks weird. Googling haven't showed up such a thing
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, FONT_ATLAS_WIDTH, FONT_ATLAS_HEIGHT, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_BYTE, texture_data);
}

static void render_unit_deinit(RenderUnit *ru)
{
    glDeleteVertexArrays(1, &(ru->vao));
    glDeleteBuffers(1, &(ru->vbo));
    glDeleteBuffers(1, &(ru->ibo));
    /* glDeleteProgram(ru->shader); */
    // Not deleting the shader here, since we only have one instance for the world.
    // NOTE @FUTURE: Probably gonna have a batch sort of thing, the guys who share the same shader
}

shader_handle_t load_shader(const char *file_path)
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
