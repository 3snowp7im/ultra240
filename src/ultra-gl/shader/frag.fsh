#version 150 core

in VS_OUT {
  vec3 texture_coords;
} fs_in;

uniform sampler2DArray tilesets;

void main() {
  gl_FragColor = texelFetch(tilesets, ivec3(fs_in.texture_coords), 0);
}
