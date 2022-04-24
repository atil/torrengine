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

    u32 vertex_shader_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_handle, 1, (const char *const *)&vert_string, NULL);
    glCompileShader(vertex_shader_handle);
    i32 success;
    glGetShaderiv(vertex_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertex_shader_handle, 512, NULL, info_log);
        printf("vertex shader fail %s\n", info_log);
    }

    u32 frag_shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
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
    i32 loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform matrix: %s\n", uniform_name);
        return;
    }
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat->data);
}

static void shader_set_float(shader_handle_t shader, const char *uniform_name, f32 f0, f32 f1, f32 f2)
{
    i32 loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform f323: %s\n", uniform_name);
        return;
    }
    glUniform3f(loc, f0, f1, f2);
}

static void shader_set_int(shader_handle_t shader, const char *uniform_name, i32 i)
{
    i32 loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform int: %s\n", uniform_name);
        return;
    }
    glUniform1i(loc, i);
}

static void shader_set_f32(shader_handle_t shader, const char *uniform_name, f32 f)
{
    i32 loc = glGetUniformLocation(shader, uniform_name);
    if (loc == -1)
    {
        printf("error setting uniform int: %s\n", uniform_name);
        return;
    }
    glUniform1f(loc, f);
}

