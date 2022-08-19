#pragma once

class Shader {
    shader_handle handle;

  public:
    explicit Shader(const std::string &file_path);
    ~Shader();

    void use();
    void set_mat4(const std::string &uniform_name, const struct Mat4 &mat);
    void set_float(const std::string &uniform_name, f32 f0, f32 f1, f32 f2);
    void set_int(const std::string &uniform_name, i32 i);
    void set_f32(const std::string &uniform_name, f32 f);
};
