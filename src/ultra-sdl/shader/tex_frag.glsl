#version 420 core
layout(location = 0) out vec4 out_color;

in vec3 frag_texture_coords;
in float frag_alpha;

uniform sampler2DArray tilesets;

void main() {
  out_color = texelFetch(tilesets, ivec3(frag_texture_coords), 0);
  out_color.a *= frag_alpha;
}
