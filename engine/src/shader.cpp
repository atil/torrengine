#define GLEW_STATIC // Statically linking glew

#include "common.h"

DISABLE_WARNINGS
#include <string>
#include <GL/glew.h>
ENABLE_WARNINGS

#include "util.h"
#include "shader.h"
#include "tomath.h"

Shader::Shader(const std::string &file_path) {

    char info_log[512]; // TODO @CLEANUP: Better logging
    const char *shader_string = (const char *)Util::read_file(file_path.c_str());

    std::string vert_string = "#version 420\n#define VERTEX\n" + std::string(shader_string);
    std::string frag_string = "#version 420\n#define FRAGMENT\n" + std::string(shader_string);

    const char *vert_string_ptr = vert_string.c_str();
    const char *frag_string_ptr = frag_string.c_str();

    u32 vertex_shader_handle = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader_handle, 1, (const char *const *)&vert_string_ptr, NULL);
    glCompileShader(vertex_shader_handle);
    i32 success;
    glGetShaderiv(vertex_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertex_shader_handle, 512, NULL, info_log);
        printf("vertex shader fail %s\n", info_log);
    }

    u32 frag_shader_handle = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag_shader_handle, 1, (const char *const *)&frag_string_ptr, NULL);
    glCompileShader(frag_shader_handle);
    glGetShaderiv(frag_shader_handle, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(frag_shader_handle, 512, NULL, info_log);
        printf("frag shader fail %s\n", info_log);
    }

    handle = glCreateProgram();
    glAttachShader(handle, vertex_shader_handle);
    glAttachShader(handle, frag_shader_handle);
    glLinkProgram(handle);
    glGetProgramiv(handle, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(handle, 512, NULL, info_log);
        printf("shader link fail %s\n", info_log);
    }
    glDeleteShader(vertex_shader_handle);
    glDeleteShader(frag_shader_handle);
}

Shader::~Shader() {
    glDeleteProgram(handle);
}

void Shader::use() {
    glUseProgram(handle);
}

void Shader::set_mat4(const std::string &uniform_name, const Mat4 &mat) {
    use();
    i32 loc = glGetUniformLocation(handle, uniform_name.c_str());
    if (loc == -1) {
        printf("error setting uniform matrix: %s\n", uniform_name.c_str());
        return;
    }

    glUniformMatrix4fv(loc, 1, GL_FALSE, mat.data);
}

void Shader::set_float(const std::string &uniform_name, f32 f0, f32 f1, f32 f2) {
    use();
    i32 loc = glGetUniformLocation(handle, uniform_name.c_str());
    if (loc == -1) {
        printf("error setting uniform f323: %s\n", uniform_name.c_str());
        return;
    }
    glUniform3f(loc, f0, f1, f2);
}

void Shader::set_int(const std::string &uniform_name, i32 i) {
    use();
    i32 loc = glGetUniformLocation(handle, uniform_name.c_str());
    if (loc == -1) {
        printf("error setting uniform int: %s\n", uniform_name.c_str());
        return;
    }
    glUniform1i(loc, i);
}

void Shader::set_f32(const std::string &uniform_name, f32 f) {
    use();
    i32 loc = glGetUniformLocation(handle, uniform_name.c_str());
    if (loc == -1) {
        printf("error setting uniform int: %s\n", uniform_name.c_str());
        return;
    }
    glUniform1f(loc, f);
}