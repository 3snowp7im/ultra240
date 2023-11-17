#define GL_GLEXT_PROTOTYPES
#include <cstring>
#include <GL/gl.h>
#include <GL/glext.h>
#include <list>
#include <memory>
#include <queue>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <ultra240/renderer.h>
#include "image.h"
#include "renderer.h"
#include "shader/shader.h"

#define BUFFERS_COUNT   3
#define TEXTURES_COUNT  3
#define MAX_SPRITES     512

namespace ultra::renderer {

  // Array indexes for each data buffer.
  enum {
    BUFFER_IDX_TILE,
    BUFFER_IDX_SPRITE,
  };

  // Array indices for each texture array.
  enum {
    TEXTURE_IDX_TILE,
    TEXTURE_IDX_ANIMATION,
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

  struct Sprite {
    const Entity* entity;
    const Texture* texture;
  };

  struct AnimationHeader {
    uint16_t tile_index;
    uint16_t tile_count;
  };

  struct AnimationTile {
    uint16_t tile_index;
    uint16_t duration;
  };

  struct Animation {
    AnimationHeader header;
    AnimationTile tiles[255];
  };

  struct TilesetHandle {
    TextureList::iterator begin;
    size_t count;
    std::unordered_map<std::string, const Texture*> texture_map;
  };

  struct EntityHandle {
    std::list<Sprite>::iterator begin;
    size_t count;
  };

  struct SpriteAttributes {
    float position[2];
    uint8_t index;
    uint16_t tile_index;
    uint8_t texture_coords_tl[2];
    uint8_t texture_coords_tr[2];
    uint8_t texture_coords_bl[2];
    uint8_t texture_coords_br[2];
  };

  class Shader {
  public:

    Shader(
      GLenum type,
      const std::string& name,
      unsigned char* shader_source,
      unsigned int shader_source_len
    ) : handle(glCreateShader(type)) {
      if (!handle) {
        throw std::runtime_error("Window: could not create " + name);
      }
      glShaderSource(
        handle,
        1,
        reinterpret_cast<GLchar**>(&shader_source),
        reinterpret_cast<GLint*>(&shader_source_len)
      );
      glCompileShader(handle);
      GLint success;
      glGetShaderiv(handle, GL_COMPILE_STATUS, &success);
      if (!success) {
        GLint log_length;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetShaderInfoLog(handle, log_length, nullptr, log);
        throw std::runtime_error(
          "Shader: could not compile " + name + ": " + log
        );
      }
    }

    ~Shader() {
      if (handle) {
        glDeleteShader(handle);
      }
    }

    GLuint handle;

    friend class Program;
  };

  class Program {
  public:

    Program(const std::string& name, const std::vector<const Shader*>& shaders)
      : handle(glCreateProgram()) {
      if (!handle) {
        throw std::runtime_error("Window: could not create " + name);
      }
      for (const auto& shader : shaders) {
        glAttachShader(handle, shader->handle);
      }
      glLinkProgram(handle);
      GLint success;
      glGetProgramiv(handle, GL_LINK_STATUS, &success);
      if (!success) {
        GLint log_length;
        glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetProgramInfoLog(handle, log_length, nullptr, log);
        throw std::runtime_error(
          "Program: could not link " + name + ": " + log
        );
      }
    }

    ~Program() {
      glDeleteProgram(handle);
    }

    void use() {
      glUseProgram(handle);
    }

    GLint get_uniform_location(const GLchar* name) {
      return glGetUniformLocation(handle, name);
    }

    GLuint handle;
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
      glGenTextures(count, &handles[0]);
    }

    ~Textures() {
      glDeleteTextures(handles.size(), &handles[0]);
    }
  };

  class Buffers : public Handles {
  public:

    Buffers(GLsizei count)
      : Handles(count) {
      glGenBuffers(count, &handles[0]);
    }

    ~Buffers() {
      glDeleteBuffers(handles.size(), &handles[0]);
    }
  };

  class VertexArrays : public Handles  {
  public:

    VertexArrays(GLsizei count)
      : Handles(count) {
      glGenVertexArrays(count, &handles[0]);
    }

    ~VertexArrays() {
      glDeleteVertexArrays(handles.size(), &handles[0]);
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
        camera_position({0.f, 0.f}),
        world_textures_count(0),
        time(0){

      // Compile shaders.
      auto tile_vert_shader = std::make_unique<Shader>(
        GL_VERTEX_SHADER,
        "tile vertex shader",
        shader_tile_vert_glsl,
        shader_tile_vert_glsl_len
      );
      auto sprite_vert_shader = std::make_unique<Shader>(
        GL_VERTEX_SHADER,
        "sprite vertex shader",
        shader_sprite_vert_glsl,
        shader_sprite_vert_glsl_len
      );
      auto tex_geom_shader = std::make_unique<Shader>(
        GL_GEOMETRY_SHADER,
        "texture geometry shader",
        shader_tex_geom_glsl,
        shader_tex_geom_glsl_len
      );
      auto tex_frag_shader = std::make_unique<Shader>(
        GL_FRAGMENT_SHADER,
        "texture fragment shader",
        shader_tex_frag_glsl,
        shader_tex_frag_glsl_len
      );

      // Link tile shader program.
      tile_program.reset(
        new Program("tile program", {
          tile_vert_shader.get(),
          tex_geom_shader.get(),
          tex_frag_shader.get(),
        })
      );

      // Link sprite shader program.
      sprite_program.reset(
        new Program("sprite program", {
          sprite_vert_shader.get(),
          tex_geom_shader.get(),
          tex_frag_shader.get(),
        })
      );

      // Delete shaders.
      tile_vert_shader.reset(nullptr);
      sprite_vert_shader.reset(nullptr);
      tex_geom_shader.reset(nullptr);
      tex_frag_shader.reset(nullptr);

      // Generate textures.
      textures.reset(new Textures(TEXTURES_COUNT));

      // Get tile uniform locations.
      for (int i = 0; i < 16; i++) {
        uniform_locations.tile.tileset_indices[i] =
          tile_program->get_uniform_location(
            ("tileset_indices[" + std::to_string(i) + "]").c_str()
          );
        uniform_locations.tile.tileset_sizes[i] = 
          tile_program->get_uniform_location(
            ("tileset_sizes[" + std::to_string(i) + "]").c_str()
          );
        uniform_locations.tile.layer_parallax[i] =
          tile_program->get_uniform_location(
            ("layer_parallax[" + std::to_string(i) + "]").c_str()
          );
      }
      uniform_locations.tile.camera_position =
        tile_program->get_uniform_location("camera_position");
      uniform_locations.tile.map_size =
        tile_program->get_uniform_location("map_size");
      uniform_locations.tile.time =
        tile_program->get_uniform_location("time");
      uniform_locations.tile.animations =
        tile_program->get_uniform_location("animations");
      uniform_locations.tile.tilesets =
        tile_program->get_uniform_location("tilesets");

      // Get sprite uniform locations.
      for (int i = 0; i < 48; i++) {
        uniform_locations.sprite.tileset_indices[i] =
          sprite_program->get_uniform_location(
            ("tileset_indices[" + std::to_string(i) + "]").c_str()
          );
        uniform_locations.sprite.tile_sizes[i] =
          sprite_program->get_uniform_location(
            ("tile_sizes[" + std::to_string(i) + "]").c_str()
          );
        uniform_locations.sprite.tileset_sizes[i] =
          sprite_program->get_uniform_location(
            ("tileset_sizes[" + std::to_string(i) + "]").c_str()
          );
      }
      uniform_locations.sprite.camera_position =
        sprite_program->get_uniform_location("camera_position");
      uniform_locations.sprite.map_position =
        sprite_program->get_uniform_location("map_position");
      uniform_locations.sprite.layer_index =
        sprite_program->get_uniform_location("layer_index");
      uniform_locations.sprite.sprite_count =
        sprite_program->get_uniform_location("sprite_count");
      uniform_locations.sprite.tilesets =
        sprite_program->get_uniform_location("tilesets");

      // Generate array buffers.
      array_buffers.reset(new Buffers(BUFFERS_COUNT));

      // Generate vertex array.
      vertex_arrays.reset(new VertexArrays(BUFFERS_COUNT));

      // Instantiate sprite buffer.
      glBindBuffer(GL_ARRAY_BUFFER, array_buffers[BUFFER_IDX_SPRITE]);
      glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(sprite_data),
        nullptr,
        GL_DYNAMIC_DRAW
      );

      // Setup the tile texture array.
      glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_TILE]);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, 2048, 2048, 64);

      // Setup the animations texture array.
      glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_ANIMATION]);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RG16UI, 256, 256, 64);

      // Push available texture indices onto the sprite texture index stack.
      for (int i = 0; i < 64; i++) {
        texture_indices.push(i);
      }
      for (int i = 0; i < 48; i++) {
        sprite_indices.push(i);
      }
    }

    const TilesetHandle* load_tilesets(
      const std::vector<const Tileset*>& tilesets
    ) {
      auto begin = add_tilesets(
        &tilesets[0],
        tilesets.size(),
        Texture::Type::Sprite
      );
      std::unordered_map<std::string, const Texture*> texture_map;
      auto it = begin;
      for (int i = 0; i < tilesets.size(); i++) {
        texture_map.insert({it->tileset->source, &*it});
        it++;
      }
      TilesetHandle* handle = new TilesetHandle {
        .begin = begin,
        .count = tilesets.size(),
        .texture_map = texture_map,
      };
      tileset_handles.insert({handle, std::unique_ptr<TilesetHandle>(handle)});
      return handle;
    }

    void unload_tilesets(
      const std::vector<const TilesetHandle*>& handles
    ) {
      for (const auto handle : handles) {
        remove_textures(handle->begin, handle->count);
        tileset_handles.erase(handle);
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

    const TilesetHandle* load_map(uint16_t index) {
      map_index = index;
      // Upload tile data.
      size_t map_area = maps[map_index].size.x * maps[map_index].size.y;
      size_t layer_size = map_area * sizeof(uint16_t);
      size_t layer_count = maps[map_index].layers.size();
      size_t buffer_size = layer_count * layer_size;
      glBindBuffer(GL_ARRAY_BUFFER, array_buffers[BUFFER_IDX_TILE]);
      glBufferData(GL_ARRAY_BUFFER, buffer_size, nullptr, GL_STATIC_DRAW);
      for (int i = 0; i < layer_count; i++) {
        glBufferSubData(
          GL_ARRAY_BUFFER,
          static_cast<GLintptr>(i * layer_size),
          static_cast<GLsizeiptr>(layer_size),
          &maps[map_index].layers[i].tiles[0]
        );
      }
      return &map_tileset_handles[index];
    }

    const EntityHandle* load_entities(
      const std::vector<const Entity*>& entities,
      const std::vector<const TilesetHandle*>& tilesets
    ) {
      // Combine the texture maps of the tileset handles.
      std::unordered_map<std::string, const Texture*> texture_map;
      for (const auto tileset : tilesets) {
        texture_map.insert(
          tileset->texture_map.cbegin(),
          tileset->texture_map.cend()
        );
      }
      // Add entities.
      auto begin = sprites.insert(sprites.end(), entities.size(), {});
      auto it = begin;
      for (const auto entity : entities) {
        *it++ = {
          .entity = entity,
          .texture = texture_map.at(entity->tileset.source),
        };
      }
      EntityHandle* handle = new EntityHandle {
        .begin = begin,
        .count = entities.size(),
      };
      entity_handles.insert({handle, std::unique_ptr<EntityHandle>(handle)});
      return handle;
    }

    void unload_entities(
      const std::vector<const EntityHandle*>& handles
    ) {
      for (const auto handle : handles) {
        auto end = handle->begin;
        for (int i = 0; i < handle->count; i++) {
          end++;
        }
        sprites.erase(handle->begin, end);
        entity_handles.erase(handle);
      }
    }

    void render() {
      // Enable depth test
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS); 

      // Enable alpha blending.
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // Render background tiles.
      render_tiles(0, maps[map_index].entities_index);

      // Use the sprite program.
      sprite_program->use();
      // Set the tileset uniform.
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_TILE]);
      glUniform1ui(uniform_locations.sprite.tilesets, 0);
      // Set uniforms linking texture index with tileset index.
      uint8_t sprite_indices[64];
      for (auto& pair : sprite_textures) {
        sprite_indices[pair.first->index] = pair.second;
        // Set the tileset indices uniform values.
        glUniform1ui(
          uniform_locations.sprite.tileset_indices[pair.second],
          pair.first->index
        );
        // Set the tile size uniform values.
        glUniform2ui(
          uniform_locations.sprite.tile_sizes[pair.second],
          pair.first->tileset->tile_size.x,
          pair.first->tileset->tile_size.y
        );
        // Set the tileset size uniform values.
        glUniform2ui(
          uniform_locations.sprite.tileset_sizes[pair.second],
          pair.first->size.x,
          pair.first->size.y
        );
      }
      // Set the camera position uniform value.
      glUniform2f(
        uniform_locations.sprite.camera_position,
        camera_position.x,
        camera_position.y
      );
      // Set the map position uniform value.
      glUniform2i(
        uniform_locations.sprite.map_position,
        maps[map_index].position.x,
        maps[map_index].position.y
      );
      // Set the layer index uniform value.
      glUniform1ui(
        uniform_locations.sprite.layer_index,
        maps[map_index].entities_index
      );
      // Set the sprite count uniform value.
      glUniform1ui(
        uniform_locations.sprite.sprite_count,
        sprites.size()
      );
      // Upload the sprite attributes data.
      SpriteAttributes* curr_attrs = sprite_data;
      for (const auto& sprite : sprites) {
        SpriteAttributes attrs = {
          .position = {sprite.entity->position.x, sprite.entity->position.y},
          .index = sprite_indices[sprite.texture->index],
          .tile_index = sprite.entity->tile_index,
          .texture_coords_tl = {0, 0},
          .texture_coords_tr = {1, 0},
          .texture_coords_bl = {0, 1},
          .texture_coords_br = {1, 1},
        };
        if (sprite.entity->attributes.flip_x) {
          attrs.texture_coords_tl[0] = 1;
          attrs.texture_coords_tr[0] = 0;
          attrs.texture_coords_bl[0] = 1;
          attrs.texture_coords_br[0] = 0;
        }
        if (sprite.entity->attributes.flip_y) {
          attrs.texture_coords_tl[1] = 1;
          attrs.texture_coords_tr[1] = 1;
          attrs.texture_coords_bl[1] = 0;
          attrs.texture_coords_br[1] = 0;
        }
        *curr_attrs++ = attrs;
      }
      glBindVertexArray(vertex_arrays[BUFFER_IDX_SPRITE]);
      glBindBuffer(GL_ARRAY_BUFFER, array_buffers[BUFFER_IDX_SPRITE]);
      glBufferSubData(
        GL_ARRAY_BUFFER,
        static_cast<GLintptr>(0),
        static_cast<GLsizeiptr>(sprites.size() * sizeof(SpriteAttributes)),
        sprite_data
      );
      // Draw the sprites.
      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, position))
      );
      glVertexAttribIPointer(
        1,
        1,
        GL_UNSIGNED_BYTE,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, index))
      );
      glVertexAttribIPointer(
        2,
        1,
        GL_UNSIGNED_SHORT,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, tile_index))
      );
      glVertexAttribIPointer(
        3,
        2,
        GL_UNSIGNED_BYTE,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, texture_coords_tl))
      );
      glVertexAttribIPointer(
        4,
        2,
        GL_UNSIGNED_BYTE,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, texture_coords_tr))
      );
      glVertexAttribIPointer(
        5,
        2,
        GL_UNSIGNED_BYTE,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, texture_coords_bl))
      );
      glVertexAttribIPointer(
        6,
        2,
        GL_UNSIGNED_BYTE,
        sizeof(SpriteAttributes),
        reinterpret_cast<void*>(offsetof(SpriteAttributes, texture_coords_br))
      );
      glEnableVertexAttribArray(0);
      glEnableVertexAttribArray(1);
      glEnableVertexAttribArray(2);
      glEnableVertexAttribArray(3);
      glEnableVertexAttribArray(4);
      glEnableVertexAttribArray(5);
      glEnableVertexAttribArray(6);
      glDrawArrays(GL_POINTS, 0, sprites.size());
      glBindVertexArray(0);

      // Render foreground tiles.
      render_tiles(
        maps[map_index].entities_index,
        maps[map_index].layers.size() - maps[map_index].entities_index
      );
    }

    geometry::Vector<float> camera_position;

  private:

    TextureList::iterator add_tilesets(
      const Tileset* const* tilesets,
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
        glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_TILE]);
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
        );
        if (type == Texture::Type::Tile) {
          Animation animations[256];
          memset(animations, 0, sizeof(animations));
          Animation* animation = animations;
          for (int j = 0; j < tilesets[i]->tiles.size(); j++) {
            const auto& tile = tilesets[i]->tiles[j];
            if (tile.animation_tiles.size()) {
              animation->header.tile_index = j;
              animation->header.tile_count = tile.animation_tiles.size();
              AnimationTile* animation_tile = animation->tiles;
              for (int k = 0; k < tile.animation_tiles.size(); k++) {
                animation_tile->tile_index = tile.animation_tiles[k].tile_index;
                animation_tile->duration = tile.animation_tiles[k].duration;
                animation_tile++;
              }
              animation++;
            }
          }
          glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_ANIMATION]);
          glTexSubImage3D(
            GL_TEXTURE_2D_ARRAY,
            0,
            0,
            0,
            texture->index,
            256,
            256,
            1,
            GL_RG_INTEGER,
            GL_UNSIGNED_SHORT,
            animations
          );
        }
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

    void render_tiles(
      size_t start_layer_idx,
      ssize_t layer_count
    ) {
      if (layer_count <= 0) {
        return;
      }
      // Use the tile program;
      tile_program->use();
      // Set the tileset uniforms.
      glUniform1i(uniform_locations.tile.tilesets, 0);
      glUniform1i(uniform_locations.tile.animations, 1);
      glActiveTexture(GL_TEXTURE0 + 0);
      glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_TILE]);
      glActiveTexture(GL_TEXTURE0 + 1);
      glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_ANIMATION]);
      // Set uniforms linking texture index with tileset index.
      auto texture = map_tile_textures[map_index].begin();
      for (int i = 0;
           texture != map_tile_textures[map_index].end();
           i++, texture++) {
        // Set the tileset indices uniform values.
        glUniform1ui(
          uniform_locations.tile.tileset_indices[i],
          (*texture)->index
        );
        // Set the tileset size uniform values.
        glUniform2ui(
          uniform_locations.tile.tileset_sizes[i],
          (*texture)->size.x,
          (*texture)->size.y
        );
      }
      // Set the layer parallax uniform values.
      for (int i = 0; i < maps[map_index].layers.size(); i++) {
        glUniform2f(
          uniform_locations.tile.layer_parallax[i],
          maps[map_index].layers[i].parallax.x,
          maps[map_index].layers[i].parallax.y
        );
      }
      // Set the camera position uniform value.
      glUniform2f(
        uniform_locations.tile.camera_position,
        camera_position.x,
        camera_position.y
      );
      // Set the map size uniform value.
      glUniform2ui(
        uniform_locations.tile.map_size,
        maps[map_index].size.x,
        maps[map_index].size.y
      );
      // Set the time uniform value.
      glUniform1ui(uniform_locations.tile.time, time);
      // Draw the tiles
      size_t map_area = maps[map_index].size.x * maps[map_index].size.y;
      size_t tile_offset = map_area * start_layer_idx;
      size_t tile_count = map_area * layer_count;
      glBindVertexArray(vertex_arrays[BUFFER_IDX_TILE]);
      glBindBuffer(GL_ARRAY_BUFFER, array_buffers[BUFFER_IDX_TILE]);
      glVertexAttribIPointer(
        0,
        1,
        GL_UNSIGNED_SHORT,
        sizeof(GLushort),
        reinterpret_cast<void*>(0)
      );
      glEnableVertexAttribArray(0);
      glDrawArrays(GL_POINTS, tile_offset, tile_count);
      glBindVertexArray(0);
    }

    std::unique_ptr<Program> tile_program;

    std::unique_ptr<Program> sprite_program;

    HandlesPointer<Buffers> array_buffers;

    HandlesPointer<VertexArrays> vertex_arrays;

    HandlesPointer<Textures> textures;

    TextureList texture_list;

    std::list<Sprite> sprites;

    std::queue<uint8_t> texture_indices;

    std::unordered_map<const Texture*, uint8_t> sprite_textures;

    std::queue<uint8_t> sprite_indices;

    std::unordered_map<
      const TilesetHandle*,
      std::unique_ptr<TilesetHandle>
    > tileset_handles;

    std::unordered_map<
      const EntityHandle*,
      std::unique_ptr<EntityHandle>
    > entity_handles;

    std::vector<std::list<Texture*>> map_tile_textures;

    std::vector<TilesetHandle> map_tileset_handles;

    TextureList::iterator world_textures;

    size_t world_textures_count;

    uint32_t time;

    struct {

      struct {
        GLint tileset_indices[16];
        GLint layer_parallax[16];
        GLint tileset_sizes[16];
        GLint camera_position;
        GLint map_size;
        GLint time;
        GLint animations;
        GLint tilesets;
      } tile;

      struct {
        GLint tileset_indices[48];
        GLint tileset_sizes[48];
        GLint tile_sizes[48];
        GLint camera_position;
        GLint map_position;
        GLint layer_index;
        GLint sprite_count;
        GLint tilesets;
      } sprite;

    } uniform_locations;

    SpriteAttributes sprite_data[MAX_SPRITES];

    const World::Map* maps;

    size_t map_index;
  };

  static std::unique_ptr<Renderer> renderer;

  void init() {
    renderer.reset(new Renderer());
  }

  void quit() {
    renderer.reset(nullptr);
  }

  void set_camera_position(const geometry::Vector<float>& position) {
    renderer->camera_position = position;
  }

  const TilesetHandle* load_tilesets(
    const std::vector<const Tileset*>& tilesets
  ) {
    return renderer->load_tilesets(tilesets);
  }

  void unload_tilesets(const std::vector<const TilesetHandle*>& handles) {
    renderer->unload_tilesets(handles);
  }

  const EntityHandle* load_entities(
    const std::vector<const Entity*>& entities,
    const std::vector<const TilesetHandle*>& tilesets
  ) {
    return renderer->load_entities(entities, tilesets);
  }

  void unload_entities(const std::vector<const EntityHandle*>& handles) {
    renderer->unload_entities(handles);
  }

  const TilesetHandle* load_map(uint16_t index) {
    return renderer->load_map(index);
  }

  void load_world(const World& world) {
    renderer->load_world(world);
  }

  void unload_world() {
    renderer->unload_world();
  }

  void render() {
    renderer->render();
  }

}
