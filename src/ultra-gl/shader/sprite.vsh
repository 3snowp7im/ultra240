#version 150 core

in uvec2 vertex;
in uint tile;
in mat3 model;

uniform uint tilesetIndices[48];
uniform uvec2 tilesetSizes[48];
uniform uvec2 tileSizes[48];
uniform mat3 view;
uniform uint layerIndex;
uniform uint spriteCount;

out VS_OUT {
  vec3 textureCoords;
} vs_out;

vec2 ultra_screenSpaceToClipSpace(
  vec2 screenSpace
) {
  const vec2 renderSize = vec2(256, 240);
  return vec2(1, -1) * (screenSpace - renderSize / 2) / (renderSize / 2);
}

vec4 ultra_getSpriteVertexPosition(
  int instanceID,
  uvec2 vertex,
  uint tile,
  mat3 model,
  mat3 view,
  uint layerIndex,
  uint spriteCount
) {
  uint textureIndex = (tile >> 16) & 0xffffu;
  // Get tile size.
  uvec2 tileSize = tileSizes[textureIndex];
  // Convert input vertex to sprite vertex.
  vec2 spriteVertex = tileSize * vertex;
  // Calculate tile screen space.
  float z = (spriteCount - uint(instanceID)) / float(1u + spriteCount);
  vec2 screenSpace =
    (view * model * vec3(spriteVertex, 1)).xy
    - uvec2(0, tileSize.y);
  // Convert screen space to clip space.
  return vec4(
    ultra_screenSpaceToClipSpace(screenSpace),
    (15u - layerIndex + z) / 16.,
    1.
  );
}

vec3 ultra_getSpriteTextureCoords(
  uvec2 vertex,
  uint tile,
  uint tilesetIndices[48],
  uvec2 tilesetSizes[48],
  uvec2 tileSizes[48]
) {
  uint textureIndex = (tile >> 16) & 0xffffu;
  uint tileIndex = tile & 0xffffu;
  // Get tile size.
  uvec2 tileSize = tileSizes[textureIndex];
  // Convert input vertex to sprite vertex.
  vec2 spriteVertex = tileSize * vertex;
  // Get tileset index and size.
  uint tilesetIndex = tilesetIndices[textureIndex];
  uvec2 tilesetSize = tilesetSizes[textureIndex];
  // Calculate tileset column count.
  uint tilesetColumns = tilesetSize.x / tileSize.x;
  // Calculate texture space.
  vec2 textureSpace = vec2(
    (tileIndex % tilesetColumns) * tileSize.x,
    tilesetSize.y - (tileIndex / tilesetColumns) * tileSize.y
  ) + vec2(spriteVertex.x, tileSize.y - spriteVertex.y - tileSize.y);
  return vec3(textureSpace, tilesetIndex);
}

void main() {
  gl_Position = ultra_getSpriteVertexPosition(
    gl_InstanceID,
    vertex,
    tile,
    model,
    view,
    layerIndex,
    spriteCount
  );
  vs_out.textureCoords = ultra_getSpriteTextureCoords(
    vertex,
    tile,
    tilesetIndices,
    tilesetSizes,
    tileSizes
  );
}
