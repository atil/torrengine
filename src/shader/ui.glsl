#ifdef VERTEX
layout(location = 0) in vec2 u_pos;
layout(location = 1) in vec2 u_texcoord;

out vec2 v2f_texcoord;

void main()
{
    v2f_texcoord = vec2(u_texcoord.x, u_texcoord.y);
    gl_Position = vec4(u_pos, 0.0, 1.0);
}
#endif

#ifdef FRAGMENT
layout(binding = 0) uniform sampler2D u_texture_ui;

in vec2 v2f_texcoord;

out vec4 out_color;

void main()
{
    float texture_value = texture(u_texture_ui, vec2(v2f_texcoord.x, -v2f_texcoord.y)).r;
    out_color = vec4(1, 1, 1, texture_value);
}
#endif
