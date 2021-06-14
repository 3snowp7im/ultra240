#version 420 core
layout (points) in;
layout (line_strip, max_vertices = 5) out;

const vec2 screen_size = vec2(256, 240);

in VS_OUT {
  vec2 size;
} geom_in[];

vec2 map_transform(vec2 pos) {
  return vec2(1.0, -1.0) * (pos - screen_size / 2) / (screen_size / 2);
}

void main() {
  vec4 pos = gl_in[0].gl_Position;
  vec2 size = geom_in[0].size;
  vec2 x = vec2(size.x, 0);
  vec2 y = vec2(0, size.y);
  gl_Position = vec4(map_transform(pos.xy), pos.zw);
  EmitVertex();
  gl_Position = vec4(map_transform(pos.xy + x), pos.zw);
  EmitVertex();
  gl_Position = vec4(map_transform(pos.xy + x + y), pos.zw);
  EmitVertex();
  gl_Position = vec4(map_transform(pos.xy + y), pos.zw);
  EmitVertex();
  gl_Position = vec4(map_transform(pos.xy), pos.zw);
  EmitVertex();
}
