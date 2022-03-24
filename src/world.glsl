#ifdef VERTEX
in vec3 in_pos;
in vec2 in_texcoord; // NOTE: Unused for now

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

out vec2 v2f_texcoord;

void main()
{
    v2f_texcoord = in_texcoord;
    gl_Position = u_proj * u_view * u_model * vec4(in_pos, 1.0);
}
#endif

#ifdef FRAGMENT

in vec2 v2f_texcoord;
layout(binding=0) uniform sampler2D u_texture;

uniform vec3 u_rectcolor;

out vec4 frag_color;

void main() 
{
    vec4 color = texture(u_texture, vec2(v2f_texcoord.x, v2f_texcoord.y));
    //frag_color = vec4(u_rectcolor, 1.0);
    frag_color = color;
};
#endif
