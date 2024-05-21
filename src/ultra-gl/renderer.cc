#define GL_GLEXT_PROTOTYPES
#include <cstring>
#include <GL/gl.h>
#include <GL/glext.h>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <unordered_map>
#include <ultra240/renderer.h>
#include "ultra/ultra.h"
#include "mat4.c"

namespace ultra::renderer::shader {
#include "shader/vert.c"
#include "shader/frag.c"
}

static const char* glEnumName(GLenum _enum) {
#define GLENUM(_ty) case _ty: return #_ty
  switch (_enum) {
    GLENUM(GL_TEXTURE);
    GLENUM(GL_RENDERBUFFER);
    GLENUM(GL_INVALID_ENUM);
    GLENUM(GL_INVALID_FRAMEBUFFER_OPERATION);
    GLENUM(GL_INVALID_VALUE);
    GLENUM(GL_INVALID_OPERATION);
    GLENUM(GL_OUT_OF_MEMORY);
    GLENUM(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
    GLENUM(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
    GLENUM(GL_FRAMEBUFFER_UNSUPPORTED);
  }
#undef GLENUM
  return "<GLenum?>";
}

#define GL_CHECK(expr) {                                \
    expr;                                               \
    GLenum gl_err = glGetError();                       \
    if (gl_err != 0) {                                  \
      std::string gl_err_str(glEnumName(gl_err));       \
      std::string msg = #expr "; GL error "             \
        + std::to_string(gl_err)                        \
        + ": " + gl_err_str;                            \
      throw error(__FILE__, __LINE__, msg);             \
    }                                                   \
  }

namespace ultra::renderer {

  enum {
    TEXTURE_IDX_TILESETS,
    TEXTURE_COUNT,
  };

  struct Texture {
    const Tileset* tileset;
    enum Type {
      Tile,
      Sprite,
    } type;
    geometry::Vector<uint32_t> size;
    GLubyte index;
  };

  using TextureList = std::list<Texture>;

  struct SpriteTexture {
    const Sprite* sprite;
    const Texture* texture;
  };

  struct TilesetHandle {
    TextureList::iterator begin;
    size_t count;
    std::unordered_map<std::string, const Texture*> texture_map;
  };

  struct SpriteHandle {
    std::list<SpriteTexture>::iterator begin;
    size_t count;
  };

  class Handles {
  public:

    Handles(GLsizei count)
      : handles(count) {}

    GLuint operator[](size_t i) {
      return handles[i];
    }

  protected:

    std::vector<GLuint> handles;
  };

  class Textures : public Handles {
  public:

    Textures(GLsizei count)
      : Handles(count) {
      GL_CHECK(glGenTextures(count, &handles[0]));
    }

    ~Textures() {
      glDeleteTextures(handles.size(), &handles[0]);
    }
  };

  template<typename T>
  class HandlesPointer : public std::unique_ptr<T> {
  public:

    HandlesPointer()
      : std::unique_ptr<T>() {}

    HandlesPointer(T* handles)
      : std::unique_ptr<T>(handles) {}

    GLuint operator[](size_t i) {
      return (*this->get())[i];
    }
  };

  class Renderer {
  public:

    Renderer()
      : maps(nullptr),
        world_textures_count(0) {

      // Generate textures.
      textures.reset(new Textures(TEXTURE_COUNT));

      // Setup the tileset texture.
      GL_CHECK(
        glBindTexture(
          GL_TEXTURE_2D_ARRAY,
          textures[TEXTURE_IDX_TILESETS]
        )
      );
      GL_CHECK(
        glTexParameteri(
          GL_TEXTURE_2D_ARRAY,
          GL_TEXTURE_MIN_FILTER,
          GL_NEAREST
        )
      );
      GL_CHECK(
        glTexParameteri(
          GL_TEXTURE_2D_ARRAY,
          GL_TEXTURE_MAG_FILTER,
          GL_NEAREST
        )
      );
      GL_CHECK(
        glTexStorage3D(
          GL_TEXTURE_2D_ARRAY,
          1,
          GL_RGBA8,
          texture_width,
          texture_height,
          texture_count
        )
      );

      // Push available texture indices onto the sprite texture index stack.
      for (int i = 0; i < 64; i++) {
        texture_indices.push(i);
      }
      for (int i = 0; i < 48; i++) {
        sprite_indices.push(i);
      }
    }

    const TilesetHandle* load_tilesets(
      const Tileset* tilesets,
      size_t tilesets_count
    ) {
      const Tileset* ptrs[tilesets_count];
      for (size_t i = 0; i < tilesets_count; i++) {
        ptrs[i] = &tilesets[i];
      }
      auto begin = add_tilesets(
        ptrs,
        tilesets_count,
        Texture::Type::Sprite
      );
      std::unordered_map<std::string, const Texture*> texture_map;
      auto it = begin;
      for (int i = 0; i < tilesets_count; i++) {
        texture_map.insert({it->tileset->source, &*it});
        it++;
      }
      TilesetHandle* handle = new TilesetHandle {
        .begin = begin,
        .count = tilesets_count,
        .texture_map = texture_map,
      };
      tileset_handles.insert({handle, std::unique_ptr<TilesetHandle>(handle)});
      return handle;
    }

    void unload_tilesets(
      const TilesetHandle* handles[],
      size_t handles_count
    ) {
      for (size_t i = 0; i < handles_count; i++) {
        remove_textures(handles[i]->begin, handles[i]->count);
        tileset_handles.erase(handles[i]);
      }
    }

    void load_world(const World& world) {
      unload_world();
      maps = &world.maps[0];
      // Map sources to tilesets.
      std::unordered_map<std::string, Tileset*> map_tileset_map;
      std::unordered_map<std::string, Tileset*> entity_tileset_map;
      for (const auto& map : world.maps) {
        for (const auto& tileset : map.map_tilesets) {
          map_tileset_map.emplace(tileset->source, tileset.get());
        }
        for (const auto& tileset : map.entity_tilesets) {
          entity_tileset_map.emplace(tileset->source, tileset.get());
        }
      }
      const Tileset** curr;
      const Tileset* map_tilesets[map_tileset_map.size()];
      curr = map_tilesets;
      for (const auto& pair : map_tileset_map) {
        *curr++ = pair.second;
      }
      const Tileset* entity_tilesets[entity_tileset_map.size()];
      curr = entity_tilesets;
      for (const auto& pair : entity_tileset_map) {
        *curr++ = pair.second;
      }
      // Map sources to textures.
      std::unordered_map<std::string, Texture*> texture_map;
      {
        auto it = add_tilesets(
          map_tilesets,
          map_tileset_map.size(),
          Texture::Type::Tile
        );
        for (int i = 0; i < map_tileset_map.size(); i++) {
          texture_map.insert({it->tileset->source, &*it});
          it++;
        }
      }
      {
        auto it = add_tilesets(
          entity_tilesets,
          entity_tileset_map.size(),
          Texture::Type::Sprite
        );
        for (int i = 0; i < entity_tileset_map.size(); i++) {
          texture_map.insert({it->tileset->source, &*it});
          it++;
        }
      }
      // Compile lists of textures for each map.
      map_tile_textures.resize(world.maps.size());
      map_tileset_handles.resize(world.maps.size());
      for (int i = 0; i < world.maps.size(); i++) {
        map_tile_textures[i].clear();
        map_tileset_handles[i].texture_map.clear();
        for (int j = 0; j < world.maps[i].map_tilesets.size(); j++) {
          const auto& source = world.maps[i].map_tilesets[j]->source;
          map_tile_textures[i].push_back(texture_map[source]);
        }
        for (int j = 0; j < world.maps[i].entity_tilesets.size(); j++) {
          const auto& source = world.maps[i].entity_tilesets[j]->source;
          map_tileset_handles[i].texture_map.insert({
            world.maps[i].entity_tilesets[j]->source,
            texture_map[source]
          });
        }
      }
    }

    void unload_world() {
      // Clear maps pointer.
      maps = nullptr;
      // Clear tile textures.
      map_tile_textures.clear();
      // Clear tileset handles.
      map_tileset_handles.clear();
      // Delete world's textures.
      remove_textures(world_textures, world_textures_count);
    }

    const TilesetHandle* set_map(uint16_t index) {
      map_index = index;
      return &map_tileset_handles[index];
    }

    const SpriteHandle* load_sprites(
      const Sprite* sprites,
      size_t sprites_count,
      const TilesetHandle* tilesets[],
      size_t tilesets_count
    ) {
      // Combine the texture maps of the tileset handles.
      std::unordered_map<std::string, const Texture*> texture_map;
      for (size_t i = 0; i < tilesets_count; i++) {
        texture_map.insert(
          tilesets[i]->texture_map.cbegin(),
          tilesets[i]->texture_map.cend()
        );
      }
      // Add sprites.
      auto begin = this->sprites.insert(
        this->sprites.end(),
        sprites_count,
        {}
      );
      auto it = begin;
      for (size_t i = 0; i < sprites_count; i++) {
        *it++ = {
          .sprite = &sprites[i],
          .texture = texture_map.at(sprites[i].tileset.source),
        };
      }
      SpriteHandle* handle = new SpriteHandle {
        .begin = begin,
        .count = sprites_count,
      };
      sprite_handles.insert({handle, std::unique_ptr<SpriteHandle>(handle)});
      return handle;
    }

    void unload_sprites(
      const SpriteHandle* handles[],
      size_t handles_count
    ) {
      for (size_t i = 0; i < handles_count; i++) {
        auto end = handles[i]->begin;
        for (size_t j = 0; j < handles[i]->count; j++) {
          end++;
        }
        sprites.erase(handles[i]->begin, end);
        sprite_handles.erase(handles[i]);
      }
    }

    uintptr_t get_texture() {
      return textures[TEXTURE_IDX_TILESETS];
    }

    void get_view_transform(
      Transform view,
      const geometry::Vector<float>& camera_position,
      size_t layer_index
    ) {
      mat4 camera, map, transform;
      mat4_translate(
        map,
        -16.f * maps[map_index].position.x,
        -16.f * maps[map_index].position.y,
        0
      );
      mat4_translate(
        camera,
        -camera_position.x * maps[map_index].layers[layer_index].parallax.x,
        -camera_position.y * maps[map_index].layers[layer_index].parallax.y,
        0
      );
      mat4_identity(transform);
      mat4_mult_mat4(transform, transform, camera);
      mat4_mult_mat4(transform, transform, map);
      memcpy(view, transform, sizeof(transform));
    }

    void get_projection_transform(
      Transform proj
    ) {
      mat4 translate, scale, transform;
      mat4_translate(translate, -256 / 2, -240 / 2, 0);
      mat4_scale(scale, 2.f / 256, -2.f / 240, 1);
      mat4_identity(transform);
      mat4_mult_mat4(transform, transform, translate);
      mat4_mult_mat4(transform, transform, scale);
      memcpy(proj, transform, sizeof(transform));
    }

    size_t get_tile_count() {
      auto map_size = maps[map_index].size.as<size_t>();
      return map_size.x * map_size.y;
    }

    size_t get_map_transforms(
      Transform vertex_transforms[],
      Transform tex_transforms[],
      size_t transforms_count,
      size_t layer_index
    ) {
      // Get tile count.
      auto tile_count = get_tile_count();
      // Get texture indices.
      geometry::Vector<uint32_t> texture_sizes[16];
      uint8_t texture_indices[16];
      auto texture = map_tile_textures[map_index].begin();
      for (int i = 0;
           texture != map_tile_textures[map_index].end();
           i++, texture++) {
        texture_indices[i] = (*texture)->index;
        texture_sizes[i] = (*texture)->size;
      }
      // Populate transforms.
      size_t count = 0;
      for (size_t i = tile_count * layer_index;
           i < tile_count * (1 + layer_index);
           i++) {
        if (count >= transforms_count) {
          break;
        }
        auto tile = maps[map_index].tiles[i];
        if (tile) {
          get_map_vertex_transform(vertex_transforms++[0], i);
          get_map_texture_transform(
            tex_transforms++[0],
            tile,
            texture_sizes,
            texture_indices
          );
        } else {
          memset(vertex_transforms++[0], 0, sizeof(Transform));
          memset(tex_transforms++[0], 0, sizeof(Transform));
        }
        count++;
      }
      return count;
    }

    size_t get_sprite_count(
      const SpriteHandle* handles[],
      size_t handles_count
    ) {
      size_t sprite_count = 0;
      for (size_t i = 0; i < handles_count; i++) {
        sprite_count += handles[i]->count;
      }
      return sprite_count;
    }

    size_t get_sprite_transforms(
      Transform vertex_transforms[],
      Transform tex_transforms[],
      size_t transforms_count,
      const SpriteHandle* handles[],
      size_t handles_count,
      size_t layer_index
    ) {
      // Get sprite count.
      size_t sprite_count = get_sprite_count(handles, handles_count);
      // Get sprite and tileset sizes
      geometry::Vector<uint32_t> texture_sizes[64];
      uint8_t texture_indices[64];
      for (auto& pair : sprite_textures) {
        texture_sizes[pair.first->index] = pair.first->size;
        texture_indices[pair.first->index] = pair.second;
      }
      // Populate the sprite attributes data.
      size_t count = 0;
      for (size_t i = 0; i < handles_count; i++) {
        auto it = handles[i]->begin;
        for (int j = 0; j < handles[i]->count; j++, it++) {
          if (count >= transforms_count) {
            break;
          }
          const auto& sprite_texture = *it;
          get_sprite_vertex_transform(
            vertex_transforms++[0],
            sprite_texture.sprite,
            layer_index,
            count,
            sprite_count
          );
          get_sprite_texture_transform(
            tex_transforms++[0],
            sprite_texture.sprite,
            texture_sizes[sprite_texture.texture->index],
            texture_indices[sprite_texture.texture->index]
          );
          count++;
        }
      }
      return count;
    }

  private:

    void get_map_vertex_transform(
      mat4 transform,
      size_t index
    ) {
      auto map_size = maps[map_index].size.as<size_t>();
      auto map_area = map_size.x * map_size.y;
      uint32_t layer_index = index / map_area;
      auto layer_start = map_area * layer_index;
      auto layer_offset = index - layer_start;
      GLfloat layer_z = (15 - layer_index) / 16.f;
      mat4 scale, translate, map, vertex;
      mat4_scale(scale, 16, 16, layer_z);
      mat4_translate(
        translate,
        16.f * (layer_offset % map_size.x),
        16.f * (layer_offset / map_size.x),
        0
      );
      mat4_translate(
        map,
        16.f * maps[map_index].position.x,
        16.f * maps[map_index].position.y,
        0
      );
      mat4_identity(vertex);
      mat4_mult_mat4(vertex, vertex, scale);
      mat4_mult_mat4(vertex, vertex, translate);
      mat4_mult_mat4(vertex, vertex, map);
      memcpy(transform, vertex, sizeof(vertex));
    }

    void get_map_texture_transform(
      mat4 transform,
      uint16_t tile,
      const geometry::Vector<uint32_t> texture_sizes[16],
      uint8_t texture_indices[16]
    ) {
      auto tileset_index = (tile >> 12) & 0xf;
      auto tile_index = (tile & 0xfffu) - 1;
      const auto& tileset = maps[map_index].map_tilesets[tileset_index];
      const auto& tile_data = tileset->tiles[tile_index];
      if (tile_data.animation_tiles.size()) {
        auto rem = time % tile_data.animation_duration;
        uint32_t duration = 0;
        for (const auto& animation_tile : tile_data.animation_tiles) {
          duration += animation_tile.duration;
          if (rem < duration) {
            tile_index = animation_tile.tile_index;
            break;
          }
        }
      }
      auto texture_size = texture_sizes[tileset_index];
      auto pos = 16 * tile_index;
      geometry::Vector<uint16_t> tex_pos(
        pos % texture_size.x,
        (pos / texture_size.x) * 16
      );
      get_texture_transform(
        transform,
        tex_pos,
        geometry::Vector<uint32_t>(16, 16),
        texture_size,
        texture_indices[tileset_index]
      );
    }

    void get_sprite_vertex_transform(
      mat4 transform,
      const Sprite* sprite,
      size_t layer_index,
      size_t sprite_index,
      size_t sprite_count
    ) {
      auto tile_size = sprite->tileset.tile_size;
      auto sprite_z = (sprite_count - sprite_index) / (1.f + sprite_count);
      auto layer_z = (15 - layer_index + sprite_z) / 16.f;
      mat4 scale, sprite_transform, translate, vertex;
      mat4_scale(scale, tile_size.x, tile_size.y, layer_z);
      mat4_from_mat3(sprite_transform, &sprite->transform[0]);
      mat4_translate(
        translate,
        sprite->position.x,
        sprite->position.y - tile_size.y,
        0
      );
      mat4_identity(vertex);
      mat4_mult_mat4(vertex, vertex, scale);
      mat4_mult_mat4(vertex, vertex, sprite_transform);
      mat4_mult_mat4(vertex, vertex, translate);
      memcpy(transform, vertex, sizeof(vertex));
    }

    void get_sprite_texture_transform(
      mat4 texture_transform,
      const Sprite* sprite,
      const geometry::Vector<uint32_t>& texture_size,
      uint8_t texture_index
    ) {
      auto tile_size = sprite->tileset.tile_size;
      auto tile_index = sprite->tile_index;
      auto pos = tile_index * tile_size;
      auto position = geometry::Vector<uint16_t>(
        pos.x % texture_size.x,
        (pos.x / texture_size.x) * tile_size.y
      );
      mat4 to, flip, from, texture, transform;
      mat4_translate(to, -.5f, -.5f, 0);
      mat4_scale(
        flip,
        sprite->attributes.flip_x ? -1 : 1,
        sprite->attributes.flip_y ? -1 : 1,
        1
      );
      mat4_translate(from, .5f, .5f, 0);
      get_texture_transform(
        texture,
        position,
        tile_size,
        texture_size,
        texture_index
      );
      mat4_identity(transform);
      mat4_mult_mat4(transform, transform, to);
      mat4_mult_mat4(transform, transform, flip);
      mat4_mult_mat4(transform, transform, from);
      mat4_mult_mat4(transform, transform, texture);
      memcpy(texture_transform, transform, sizeof(transform));
    }

    void get_texture_transform(
      mat4 transform,
      const geometry::Vector<uint16_t>& position,
      const geometry::Vector<uint16_t>& tile_size,
      const geometry::Vector<uint32_t>& tileset_size,
      uint8_t texture_index
    ) {
      mat4 to, flip, from, scale, translate, range, texture;
      mat4_translate(to, 0, -.5f, 0);
      mat4_scale(flip, 1, -1, 1);
      mat4_translate(from, 0, .5f, 0);
      mat4_scale(scale, tile_size.x, tile_size.y, texture_index);
      mat4_translate(
        translate,
        position.x,
        tileset_size.y - position.y - tile_size.y,
        0
      );
      mat4_scale(range, 1.f / texture_width, 1.f / texture_height, 1);
      mat4_identity(texture);
      mat4_mult_mat4(texture, texture, to);
      mat4_mult_mat4(texture, texture, flip);
      mat4_mult_mat4(texture, texture, from);
      mat4_mult_mat4(texture, texture, scale);
      mat4_mult_mat4(texture, texture, translate);
      mat4_mult_mat4(texture, texture, range);
      memcpy(transform, texture, sizeof(texture));
    }

    TextureList::iterator add_tilesets(
      const Tileset* tilesets[],
      size_t tilesets_count,
      Texture::Type type
    ) {
      auto begin = texture_list.insert(texture_list.end(), tilesets_count, {});
      auto texture = begin;
      for (int i = 0; i < tilesets_count; i++) {
        Image image = Image(tilesets[i]->source);
        *texture = {
          .tileset = tilesets[i],
          .type = type,
          .size = image.size,
          .index = texture_indices.front(),
        };
        texture_indices.pop();
        GL_CHECK(
          glBindTexture(
            GL_TEXTURE_2D_ARRAY,
            textures[TEXTURE_IDX_TILESETS]
          )
        );
        GL_CHECK(
          glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0,
            0,
            0,
            texture->index,
            image.size.x,
            image.size.y,
            1,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            &image.data[0]
          )
        );
        sprite_textures.insert({&*texture, sprite_indices.front()});
        sprite_indices.pop();
        texture++;
      }
      return begin;
    }

    void remove_textures(
      TextureList::iterator begin,
      size_t count
    ) {
      if (count > 0) {
        auto it = begin;
        for (int i = 0; i < count; i++, it++) {
          texture_indices.push(it->index);
          if (it->type == Texture::Type::Sprite) {
            auto pair = sprite_textures.find(&*it);
            sprite_indices.push(pair->second);
            sprite_textures.erase(pair);
          }
        }
        texture_list.erase(begin, it);
      }
    }

    HandlesPointer<Textures> textures;

    TextureList texture_list;

    std::list<SpriteTexture> sprites;

    std::queue<uint8_t> texture_indices;

    std::unordered_map<const Texture*, uint8_t> sprite_textures;

    std::queue<uint8_t> sprite_indices;

    std::unordered_map<
      const TilesetHandle*,
      std::unique_ptr<TilesetHandle>
    > tileset_handles;

    std::unordered_map<
      const SpriteHandle*,
      std::unique_ptr<SpriteHandle>
    > sprite_handles;

    std::vector<std::list<Texture*>> map_tile_textures;

    std::vector<TilesetHandle> map_tileset_handles;

    TextureList::iterator world_textures;

    size_t world_textures_count;

    const World::Map* maps;

    size_t map_index;
  };

  static std::unique_ptr<Renderer> renderer;

  void init() {
    renderer.reset(new Renderer());
    time = 0;
  }

  void quit() {
    renderer.reset(nullptr);
  }

  const TilesetHandle* load_tilesets(
    const Tileset* tilesets,
    size_t tilesets_count
  ) {
    return renderer->load_tilesets(tilesets, tilesets_count);
  }

  void unload_tilesets(
    const TilesetHandle* handles[],
    size_t handles_count
  ) {
    renderer->unload_tilesets(handles, handles_count);
  }

  const SpriteHandle* load_sprites(
    const Sprite* sprites,
    size_t sprites_count,
    const TilesetHandle* tilesets[],
    size_t tilesets_count
  ) {
    return renderer->load_sprites(
      sprites,
      sprites_count,
      tilesets,
      tilesets_count
    );
  }

  void unload_sprites(
    const SpriteHandle* handles[],
    size_t handles_count
  ) {
    renderer->unload_sprites(handles, handles_count);
  }

  const TilesetHandle* set_map(uint16_t index) {
    return renderer->set_map(index);
  }

  void load_world(const World& world) {
    renderer->load_world(world);
  }

  void unload_world() {
    renderer->unload_world();
  }

  uintptr_t get_texture() {
    return renderer->get_texture();
  }

  void get_view_transform(
    Transform view,
    const geometry::Vector<float>& camera_position,
    size_t layer_index
  ) {
    renderer->get_view_transform(view, camera_position, layer_index);
  }

  void get_projection_transform(
    Transform proj
  ) {
    renderer->get_projection_transform(proj);
  }

  size_t get_tile_count() {
    return renderer->get_tile_count();
  }

  size_t get_map_transforms(
    Transform vertex_transforms[],
    Transform tex_transforms[],
    size_t transforms_count,
    size_t layer_index
  ) {
    return renderer->get_map_transforms(
      vertex_transforms,
      tex_transforms,
      transforms_count,
      layer_index
    );
  }

  size_t get_sprite_count(
    const SpriteHandle* handles[],
    size_t handles_count
  ) {
    return renderer->get_sprite_count(handles, handles_count);
  }

  size_t get_sprite_transforms(
    Transform vertex_transforms[],
    Transform tex_transforms[],
    size_t transforms_count,
    const SpriteHandle* handles[],
    size_t handles_count,
    size_t layer_index
  ) {
    return renderer->get_sprite_transforms(
      vertex_transforms,
      tex_transforms,
      transforms_count,
      handles,
      handles_count,
      layer_index
    );
  }

}
