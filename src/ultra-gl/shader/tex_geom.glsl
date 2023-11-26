#version 330 core
layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

#define FLIP_X  1u
#define FLIP_Y  2u

const vec2 screen_size = vec2(256, 240);

in VS_OUT {
  uvec2 size;
  uvec3 texture_pos;
  uvec2 texture_coords[4];
  mat3 transform;
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
  mat3 transform,
  float alpha
) {
  uvec3 a = uvec3(0u, 0u, 1u);
  uvec3 x = uvec3(size.x, 0u, 1u);
  uvec3 y = uvec3(0u, size.y, 1u);
  uvec3 b = uvec3(size, 1u);
  ivec2 s = ivec2(size.x, -size.y);
  // Top left.
  gl_Position = vec4(map_transform(pos.xy + (transform * a).xy), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[0]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  // Top right.
  gl_Position = vec4(map_transform(pos.xy + (transform * x).xy), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[1]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  // Bottom left.
  gl_Position = vec4(map_transform(pos.xy + (transform * y).xy), pos.zw);
  frag_texture_coords = vec3(
    ivec2(texture_pos.xy) + s * ivec2(texture_coords[2]),
    texture_pos.z
  );
  frag_alpha = alpha;
  EmitVertex();
  // Bottom right.
  gl_Position = vec4(map_transform(pos.xy + (transform * b).xy), pos.zw);
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
    geom_in[0].transform,
    geom_in[0].alpha
  );
}
