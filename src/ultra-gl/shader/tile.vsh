#version 150 core

in uvec2 vertex;
in uint tile;

uniform uint tilesetIndices[16];
uniform uvec2 tilesetSizes[16];
uniform uint startLayerIndex;
uniform vec2 layerParallax[16];
uniform vec2 view;
uniform uvec2 mapSize;
uniform uint time;
uniform usampler2DArray animations;

out VS_OUT {
  vec3 textureCoords;
} vs_out;

vec2 ultra_screenSpaceToClipSpace(
  vec2 screenSpace
) {
  const vec2 renderSize = vec2(256, 240);
  return vec2(1, -1) * (screenSpace - renderSize / 2) / (renderSize / 2);
}

vec4 ultra_getTileVertexPosition(
  int instanceID,
  uvec2 vertex,
  uint tile,
  uint tilesetIndices[16],
  uint startLayerIndex,
  vec2 layerParallax[16],
  vec2 view,
  uvec2 mapSize
) {
  if (tile == 0u) {
    return vec4(0);
  }
  uint textureIndex = (tile >> 12) & 0xfu;
  vec2 tileVertex = vec2(16u * vertex);
  uint tilesetIndex = tilesetIndices[textureIndex];
  // Convert instance ID to layer index.
  uint layerIndex = startLayerIndex + uint(instanceID) / (mapSize.x * mapSize.y);
  // Calculate tile screen space.
  vec2 screenSpace = 16 * vec2(
    (uint(instanceID) % mapSize.x),
    ((uint(instanceID) / mapSize.x) % mapSize.y)
  ) + tileVertex + view * layerParallax[layerIndex];
  // Convert screen space to clip space.
  return vec4(
    ultra_screenSpaceToClipSpace(screenSpace),
    (15u - layerIndex) / 16.,
    1.
  );
}

vec3 ultra_getTileTextureCoords(
  uvec2 vertex,
  uint tile,
  uint tilesetIndices[16],
  uvec2 tilesetSizes[16],
  uint time,
  usampler2DArray animations
) {
  if (tile == 0u) {
    return vec3(0);
  }
  uint textureIndex = (tile >> 12) & 0xfu;
  uint tileIndex = (tile & 0xfffu) - 1u;
  vec2 tileVertex = vec2(16u * vertex);
  uint tilesetIndex = tilesetIndices[textureIndex];
  uvec2 tilesetSize = tilesetSizes[textureIndex];
  // Calculate tileset column count.
  uint tilesetColumns = tilesetSize.x / 16u;
  // Check for animation.
  for (int i = 0; i < 256; i++) {
    vec4 header = texelFetch(animations, ivec3(0, i, tilesetIndex), 0);
    if (header.g == 0) {
      break;
    }
    if (tileIndex == header.r) {
      // Count duration of animation cycle.
      uint duration = 0u;
      for (int j = 0; j < header.g; j++) {
        vec4 tile = texelFetch(animations, ivec3(j + 1, i, tilesetIndex), 0);
        duration += uint(tile.g);
      }
      // Find remainder.
      uint rem = time % duration;
      // Find tile for current time.
      duration = 0u;
      for (int j = 0; j < header.g; j++) {
        vec4 tile = texelFetch(animations, ivec3(j + 1, i, tilesetIndex), 0);
        if (rem < duration + tile.g) {
          tileIndex = uint(tile.r);
          break;
        }
        duration += uint(tile.g);
      }
    }
  }
  return vec3(
    vec2(
      (tileIndex % tilesetColumns) * 16u,
      tilesetSize.y - (tileIndex / tilesetColumns) * 16u
    ) + vec2(tileVertex.x, 16 - tileVertex.y - 16),
    tilesetIndex
  );
}

void main() {
  gl_Position = ultra_getTileVertexPosition(
    gl_InstanceID,
    vertex,
    tile,
    tilesetIndices,
    startLayerIndex,
    layerParallax,
    view,
    mapSize
  );
  vs_out.textureCoords = ultra_getTileTextureCoords(
    vertex,
    tile,
    tilesetIndices,
    tilesetSizes,
    time,
    animations
  );
}
