#ifdef VERTEX
in vec3 in_pos;
in vec2 in_texcoord; // NOTE: Unused for now

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;

void main()
{
   gl_Position = u_proj * u_view * u_model * vec4(in_pos, 1.0);
}
#endif

#ifdef FRAGMENT

uniform vec3 u_rectcolor;

out vec4 frag_color;

void main() 
{
    frag_color = vec4(u_rectcolor, 1.0);
};
#endif
