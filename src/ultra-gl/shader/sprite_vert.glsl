#version 330 core
layout (location = 0) in vec2 entity_position;
layout (location = 1) in vec3 transform_col0;
layout (location = 2) in vec3 transform_col1;
layout (location = 3) in vec3 transform_col2;
layout (location = 4) in uint index;
layout (location = 5) in uint tile_index;
layout (location = 6) in uvec2 texture_coords_tl;
layout (location = 7) in uvec2 texture_coords_tr;
layout (location = 8) in uvec2 texture_coords_bl;
layout (location = 9) in uvec2 texture_coords_br;
layout (location = 10) in float opacity;

uniform uint tileset_indices[48];
uniform uvec2 tileset_sizes[48];
uniform uvec2 tile_sizes[48];
uniform vec2 camera_position;
uniform ivec2 map_position;
uniform uint layer_index;
uniform uint sprite_count;

out VS_OUT {
  uvec2 size;
  uvec3 texture_pos;
  uvec2 texture_coords[4];
  mat3 transform;
  float alpha;
} vertex_out;

void main() {
  vertex_out.transform = mat3(
    transform_col0,
    transform_col1,
    transform_col2
  );
  vertex_out.alpha = opacity;
  // Forward tile size to geometry shader.
  uvec2 tile_size = tile_sizes[index];
  vertex_out.size = tile_size;
  // Set sprite position.
  float z = (sprite_count - uint(gl_VertexID)) / float(1u + sprite_count);
  gl_Position = vec4(
    entity_position
    - map_position * 16
    - camera_position
    - uvec2(0, tile_size.y),
    (15u - layer_index + z) / 16.,
    1.
  );
  // Calculate tileset column count.
  uvec2 tileset_size = tileset_sizes[index];
  uint tileset_columns = tileset_size.x / tile_size.x;
  // Convert tile index to texture start position.
  vertex_out.texture_pos = uvec3(
    (tile_index % tileset_columns) * tile_size.x,
    tileset_size.y - (tile_index / tileset_columns) * tile_size.y,
    tileset_indices[index]
  );
  vertex_out.texture_coords[0] = texture_coords_tl;
  vertex_out.texture_coords[1] = texture_coords_tr;
  vertex_out.texture_coords[2] = texture_coords_bl;
  vertex_out.texture_coords[3] = texture_coords_br;
}
