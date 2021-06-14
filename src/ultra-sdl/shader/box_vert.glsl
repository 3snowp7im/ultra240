#version 420 core
layout (location = 0) in vec2 position;
layout (location = 1) in vec2 size;

uniform vec2 camera_position;
uniform ivec2 map_position;

out VS_OUT {
  vec2 size;
} vertex_out;

void main() {
  gl_Position = vec4(position - map_position * 16 - camera_position, 0, 1);
  vertex_out.size = size;
}
