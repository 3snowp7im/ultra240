#version 150 core

in vec3 vertex;
in mat4 quad_transform;
in mat4 texture_transform;

out VS_OUT {
  vec4 textureCoords;
} vs_out;

void main() {
  gl_Position = quad_transform * vec4(vertex, 1);
  vs_out.textureCoords = texture_transform * vec4(vertex, 1);
}
