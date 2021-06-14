#version 420 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

#define FLIP_X  1u
#define FLIP_Y  2u

const vec2 screen_size = vec2(256, 240);

in VS_OUT {
  uvec2 size;
  uvec3 texture_pos;
  uvec2 texture_coords[4];
  float alpha;
} geom_in[];

out vec3 frag_texture_coords;
out float frag_alpha;
out vec3 frag_color;

vec2 map_transform(vec2 pos) {
  return vec2(1.0, -1.0) * (pos - screen_size / 2) / (screen_size / 2);
}

void build_tile(
  vec4 pos,
  uvec2 size,
  uvec3 texture_pos,
  uvec2 texture_coords[4],
  float alpha
) {
  uvec2 x = uvec2(size.x, 0u);
  uvec2 y = uvec2(0u, size.y);
  ivec2 s = ivec2(size.x, -size.y);
  // Top left.
  gl_Position = vec4(map_transform(pos.xy), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[0]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  // Top right.
  gl_Position = vec4(map_transform(pos.xy + x), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[1]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  // Bottom left.
  gl_Position = vec4(map_transform(pos.xy + y), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[2]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  // Bottom right.
  gl_Position = vec4(map_transform(pos.xy + x + y), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[3]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  EndPrimitive();
}

void main() {
  // Emit map and texture coords.
  build_tile(
    gl_in[0].gl_Position,
    geom_in[0].size,
    geom_in[0].texture_pos,
    geom_in[0].texture_coords,
    geom_in[0].alpha
  );
}
