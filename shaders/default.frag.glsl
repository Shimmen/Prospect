#version 460

in vec2 v_uv;

uniform sampler2D u_texture;

out vec4 o_color;

void main()
{
    o_color = texture(u_texture, v_uv);
}
