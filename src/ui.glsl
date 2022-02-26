#ifdef VERTEX
layout (location = 0) in vec2 u_pos;
layout (location = 1) in vec2 u_texcoord;

out VS_OUTPUT {
    vec2 TexCoord;
} OUT;

void main()
{
    gl_Position = vec4(u_pos, 0.0, 1.0);
    OUT.TexCoord = vec2(u_texcoord.x, u_texcoord.y);
}
#endif

#ifdef FRAGMENT
layout(binding=0) uniform sampler2D u_texture_ui;

in VS_OUTPUT {
    vec2 TexCoord;
} IN; // TODO @CLEANUP: Consistent naming conventions

out vec4 Color;

void main()
{
    Color = texture(u_texture_ui, vec2(IN.TexCoord.x, -IN.TexCoord.y));
}
#endif
