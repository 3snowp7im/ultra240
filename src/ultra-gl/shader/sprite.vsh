#version 150 core

in vec3 vertex;
in mat4 model;
in mat4 texture;

uniform mat4 view;

out VS_OUT {
  vec3 textureCoords;
} vs_out;

void main() {
  gl_Position = view * model * vec4(vertex, 1);
  vs_out.textureCoords = (texture * vec4(vertex, 1)).xyz;
}
