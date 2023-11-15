#version 330 core
layout (location = 0) in uint tile_value;

uniform uint tileset_indices[16];
uniform uvec2 tileset_sizes[16];
uniform vec2 layer_parallax[16];
uniform vec2 camera_position;
uniform uvec2 map_size;
uniform uint time;
uniform usampler2DArray animations;

out VS_OUT {
  uvec2 size;
  uvec3 texture_pos;
  uvec2 texture_coords[4];
  float alpha;
} vertex_out;

void main() {
  vertex_out.alpha = float(tile_value > 0u);
  // Convert tile value to tile index.
  uint tile_index = (tile_value & 0xfffu) - 1u;
  // Convert vertex ID to layer index.
  uint layer_index = uint(gl_VertexID) / (map_size.x * map_size.y);
  // Calculate tileset index.
  uint index = tile_value >> 12;
  uint tileset_index = tileset_indices[index];
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
  // Tile size is constant.
  vertex_out.size = uvec2(16u, 16u);
  // Convert vertex ID to tile screen coords.
  gl_Position = vec4(
    (uint(gl_VertexID) % map_size.x) * 16u,
    ((uint(gl_VertexID) / map_size.x) % map_size.y) * 16u,
    (15u - layer_index) / 16.,
    1.0
  ) - vec4(camera_position * layer_parallax[layer_index], 0, 0);
  // Calculate tileset column count.
  uvec2 tileset_size = tileset_sizes[index];
  uint tileset_columns = tileset_size.x / 16u;
  // Convert tile index to texture start position.
  vertex_out.texture_pos = uvec3(
    (tile_index % tileset_columns) * 16u,
    tileset_size.y - (tile_index / tileset_columns) * 16u,
    tileset_index
  );
  vertex_out.texture_coords[0] = uvec2(0u, 0u);
  vertex_out.texture_coords[1] = uvec2(1u, 0u);
  vertex_out.texture_coords[2] = uvec2(0u, 1u);
  vertex_out.texture_coords[3] = uvec2(1u, 1u);
}
