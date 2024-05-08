#version 150 core

in VS_OUT {
  vec4 textureCoords;
} fs_in;

uniform sampler2DArray tilesets;

void main() {
  gl_FragColor = texelFetch(tilesets, ivec3(fs_in.textureCoords.xyz), 0);
}
