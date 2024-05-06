#version 150 core

in uvec2 vertex;
in uint tile;

uniform uint tileset_indices[16];
uniform uvec2 tileset_sizes[16];
uniform uint start_layer_index;
uniform vec2 layer_parallax[16];
uniform vec2 camera_position;
uniform uvec2 map_size;
uniform uint time;
uniform usampler2DArray animations;

out VS_OUT {
  vec3 texture_coords;
  float alpha;
} vs_out;

const vec2 render_size = vec2(256, 240);

void main() {
  vs_out.alpha = float(tile > 0u);
  // Convert input vertex to tile vertex.
  vec2 tile_vertex = vec2(16u * vertex);
  // Convert tile value to tile index.
  uint tile_index = (tile & 0xfffu) - 1u;
  // Convert instance ID to layer index.
  uint layer_index =
    start_layer_index
    + (uint(gl_InstanceID)) / (map_size.x * map_size.y);
  // Calculate tileset index and size.
  uint texture_index = tile >> 12;
  uint tileset_index = tileset_indices[texture_index];
  uvec2 tileset_size = tileset_sizes[texture_index];
  // Calculate tileset column count.
  uint tileset_columns = tileset_size.x / 16u;
  // Check for animation.
  for (int i = 0; i < 256; i++) {
    vec4 header = texelFetch(animations, ivec3(0, i, tileset_index), 0);
    if (header.g == 0) {
      break;
    }
    if (tile_index == header.r) {
      // Count duration of animation cycle.
      uint duration = 0u;
      for (int j = 0; j < header.g; j++) {
        vec4 tile = texelFetch(animations, ivec3(j + 1, i, tileset_index), 0);
        duration += uint(tile.g);
      }
      // Find remainder.
      uint rem = time % duration;
      // Find tile for current time.
      duration = 0u;
      for (int j = 0; j < header.g; j++) {
        vec4 tile = texelFetch(animations, ivec3(j + 1, i, tileset_index), 0);
        if (rem < duration + tile.g) {
          tile_index = uint(tile.r);
          break;
        }
        duration += uint(tile.g);
      }
    }
  }
  // Convert instance ID to tile screen space.
  vec2 screen_space = 16 * vec2(
    (uint(gl_InstanceID) % map_size.x),
    ((uint(gl_InstanceID) / map_size.x) % map_size.y)
  ) + tile_vertex - camera_position * layer_parallax[layer_index];
  // Convert screen space to clip space.
  gl_Position = vec4(
    vec2(1, -1) * (screen_space - render_size / 2) / (render_size / 2),
    (15u - layer_index) / 16.,
    1.
  );
  // Convert tile index to texture start position.
  vs_out.texture_coords = vec3(
    vec2(
      (tile_index % tileset_columns) * 16u,
      tileset_size.y - (tile_index / tileset_columns) * 16u
    ) + vec2(tile_vertex.x, 16 - tile_vertex.y - 16),
    tileset_index
  );
}
