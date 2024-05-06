#version 150 core

in uvec2 vertex;
in uint tile_index;
in uint texture_index;
in mat3 transform;
in float opacity;

uniform uint tileset_indices[48];
uniform uvec2 tileset_sizes[48];
uniform uvec2 tile_sizes[48];
uniform vec2 camera_position;
uniform ivec2 map_position;
uniform uint layer_index;
uniform uint sprite_count;

out VS_OUT {
  vec3 texture_coords;
  float alpha;
} vs_out;

const vec2 render_size = vec2(256, 240);

void main() {
  // Get tile size.
  uvec2 tile_size = tile_sizes[texture_index];
  // Convert input vertex to sprite vertex.
  vec2 sprite_vertex = tile_size * vertex;
  // Calculate tile screen space.
  float z = (sprite_count - uint(gl_InstanceID)) / float(1u + sprite_count);
  vec2 screen_space =
    (transform * vec3(sprite_vertex, 1)).xy
    - map_position * 16
    - camera_position
    - uvec2(0, tile_size.y);
  // Convert screen space to clip space.
  gl_Position = vec4(
    vec2(1, -1) * (screen_space - render_size / 2) / (render_size / 2),
    (15u - layer_index + z) / 16.,
    1.
  );
  // Calculate tileset column count.
  uint tileset_index = tileset_indices[texture_index];
  uvec2 tileset_size = tileset_sizes[texture_index];
  uint tileset_columns = tileset_size.x / tile_size.x;
  // Convert tile index to texture start position.
  vs_out.texture_coords = vec3(
    vec2(
      (tile_index % tileset_columns) * tile_size.x,
      tileset_size.y - (tile_index / tileset_columns) * tile_size.y
    ) + vec2(sprite_vertex.x, tile_size.y - sprite_vertex.y - tile_size.y),
    tileset_index
  );
  // Pass opacity through to fragment shader.
  vs_out.alpha = opacity;
}
