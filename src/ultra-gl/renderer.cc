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

#define MAX_ATTRIBUTES  16384

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

  // Array indices for each data buffer.
  enum {
    BUFFER_IDX_QUAD_VERTS,
    BUFFER_IDX_VERTEX_ATTRIB,
    BUFFER_COUNT,
  };

  // Array indices for each vertex array.
  enum {
    VERTEX_ARRAY_IDX_QUAD_VERTS,
    VERTEX_ARRAY_COUNT,
  };

  // Array indices for each texture array.
  enum {
    TEXTURE_IDX_TILESETS,
    TEXTURE_IDX_RENDER,
    TEXTURE_COUNT,
  };

  // Array indices for each framebuffer.
  enum {
    FRAMEBUFFER_IDX_RENDER,
    FRAMEBUFFER_COUNT,
  };

  // Array indices for each renderbuffer.
  enum {
    RENDERBUFFER_IDX_DEPTH,
    RENDERBUFFER_COUNT,
  };

  // Attribute locations.
  enum {
    ATTRIB_LOCATION_VERTEX = 0,
    ATTRIB_LOCATION_QUAD_TRANSFORM = 1,
    ATTRIB_LOCATION_TEXTURE_TRANSFORM = 5,
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

  struct TilesetHandle {
    TextureList::iterator begin;
    size_t count;
    std::unordered_map<std::string, const Texture*> texture_map;
  };

  struct SpriteHandle {
    std::list<Sprite>::iterator begin;
    size_t count;
  };

  struct VertexAttributes {
    mat4 quad_transform;
    mat4 texture_transform;
  };

  static const GLfloat quad_vertices[][3] = {
    {0, 0, 1},
    {1, 0, 1},
    {0, 1, 1},
    {1, 1, 1},
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
        auto msg = "could not create " + name;
        throw error(__FILE__, __LINE__, msg);
      }
      GL_CHECK(
        glShaderSource(
          handle,
          1,
          reinterpret_cast<GLchar**>(&shader_source),
          reinterpret_cast<GLint*>(&shader_source_len)
        )
      );
      GL_CHECK(glCompileShader(handle));
      GLint success;
      GL_CHECK(glGetShaderiv(handle, GL_COMPILE_STATUS, &success));
      if (!success) {
        GLint log_length;
        GL_CHECK(glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_length));
        GLchar log[log_length];
        GL_CHECK(glGetShaderInfoLog(handle, log_length, nullptr, log));
        auto msg = "could not compile " + name + ": " + log;
        throw error(__FILE__, __LINE__, msg);
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
      : name(name),
        handle(glCreateProgram()) {
      if (!handle) {
        auto msg = "could not create " + name;
        throw error(__FILE__, __LINE__, msg);
      }
      for (const auto& shader : shaders) {
        GL_CHECK(glAttachShader(handle, shader->handle));
      }
    }

    ~Program() {
      if (handle) {
        glDeleteProgram(handle);
      }
    }

    void link() {
      GL_CHECK(glLinkProgram(handle));
      GLint success;
      GL_CHECK(glGetProgramiv(handle, GL_LINK_STATUS, &success));
      if (!success) {
        GLint log_length;
        GL_CHECK(glGetProgramiv(handle, GL_INFO_LOG_LENGTH, &log_length));
        GLchar log[log_length];
        GL_CHECK(glGetProgramInfoLog(handle, log_length, nullptr, log));
        auto msg = "could not link " + name + ": " + log;
        throw error(__FILE__, __LINE__, msg);
      }
    }

    void use() {
      GL_CHECK(glUseProgram(handle));
    }

    void bind_attrib_location(GLint location, const GLchar* name) {
      GL_CHECK(glBindAttribLocation(handle, location, name));
    }

    GLint get_uniform_location(const GLchar* name) {
      GLint value;
      GL_CHECK(value = glGetUniformLocation(handle, name));
      if (value == -1) {
        std::string sname(name);
        auto msg = "cannot find uniform " + sname;
        throw error(__FILE__, __LINE__, msg);
      }
      return value;
    }

    std::string name;
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
      GL_CHECK(glGenTextures(count, &handles[0]));
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
      GL_CHECK(glGenVertexArrays(count, &handles[0]));
    }

    ~VertexArrays() {
      glDeleteVertexArrays(handles.size(), &handles[0]);
    }
  };

  class Framebuffers : public Handles {
  public:

    Framebuffers(GLsizei count)
      : Handles(count) {
      GL_CHECK(glGenFramebuffers(count, &handles[0]));
    }

    ~Framebuffers() {
      glDeleteFramebuffers(handles.size(), &handles[0]);
    }
  };


  class Renderbuffers : public Handles {
  public:

    Renderbuffers(GLsizei count)
      : Handles(count) {
      GL_CHECK(glGenRenderbuffers(count, &handles[0]));
    }

    ~Renderbuffers() {
      glDeleteRenderbuffers(handles.size(), &handles[0]);
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
        world_textures_count(0) {

      // Compile shaders.
      auto vert_shader = std::make_unique<Shader>(
        GL_VERTEX_SHADER,
        "vertex shader",
        shader::shader_vert_vsh,
        shader::shader_vert_vsh_len
      );
      auto frag_shader = std::make_unique<Shader>(
        GL_FRAGMENT_SHADER,
        "fragment shader",
        shader::shader_frag_fsh,
        shader::shader_frag_fsh_len
      );

      // Create shader program.
      program.reset(
        new Program("program", {
          vert_shader.get(),
          frag_shader.get(),
        })
      );

      // Get attribute locations.
      program->bind_attrib_location(
        ATTRIB_LOCATION_VERTEX,
        "vertex"
      );
      program->bind_attrib_location(
        ATTRIB_LOCATION_QUAD_TRANSFORM,
        "quad_transform"
      );
      program->bind_attrib_location(
        ATTRIB_LOCATION_TEXTURE_TRANSFORM,
        "texture_transform"
      );

      // Link sprite shader program.
      program->link();

      // Delete shaders.
      vert_shader.reset(nullptr);
      frag_shader.reset(nullptr);

      // Get uniform locations.
      uniform_locations.tilesets = program->get_uniform_location("tilesets");

      // Generate array buffers.
      buffers.reset(new Buffers(BUFFER_COUNT));

      // Generate vertex array.
      vertex_arrays.reset(new VertexArrays(VERTEX_ARRAY_COUNT));

      // Generate textures.
      textures.reset(new Textures(TEXTURE_COUNT));

      // Create frame buffers.
      framebuffers.reset(new Framebuffers(FRAMEBUFFER_COUNT));

      // Create depth buffers.
      renderbuffers.reset(new Renderbuffers(RENDERBUFFER_COUNT));

      // Initiate render texture.
      GL_CHECK(
        glBindTexture(
          GL_TEXTURE_2D,
          textures[TEXTURE_IDX_RENDER]
        )
      );
      GL_CHECK(
        glTexParameteri(
          GL_TEXTURE_2D,
          GL_TEXTURE_MIN_FILTER,
          GL_NEAREST
        )
      );
      GL_CHECK(
        glTexParameteri(
          GL_TEXTURE_2D,
          GL_TEXTURE_MAG_FILTER,
          GL_NEAREST
        )
      );
      GL_CHECK(
        glTexImage2D(
          GL_TEXTURE_2D,
          0,
          GL_RGBA8,
          256,
          240,
          0,
          GL_RGBA,
          GL_UNSIGNED_BYTE,
          nullptr
        )
      );

      // Initiate depth buffer.
      GL_CHECK(
        glBindRenderbuffer(
          GL_RENDERBUFFER,
          renderbuffers[RENDERBUFFER_IDX_DEPTH]
        )
      );
      GL_CHECK(
        glRenderbufferStorage(
          GL_RENDERBUFFER,
          GL_DEPTH_COMPONENT,
          256,
          240
        )
      );

      // Attach depth buffer to frame buffer.
      GL_CHECK(
        glBindFramebuffer(
          GL_FRAMEBUFFER,
          framebuffers[FRAMEBUFFER_IDX_RENDER]
        )
      );
      GL_CHECK(
        glFramebufferRenderbuffer(
          GL_FRAMEBUFFER,
          GL_DEPTH_ATTACHMENT,
          GL_RENDERBUFFER,
          renderbuffers[RENDERBUFFER_IDX_DEPTH]
        )
      );

      // Set rendered texture as color attachment 0.
      GL_CHECK(
        glFramebufferTexture(
          GL_FRAMEBUFFER,
          GL_COLOR_ATTACHMENT0,
          textures[TEXTURE_IDX_RENDER],
          0
        )
      );
      GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
      GL_CHECK(glDrawBuffers(1, &draw_buffer));
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::string msg = "could not attach frame buffer";
        throw error(__FILE__, __LINE__, msg);
      }

      // Instantiate quad vertices.
      GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_IDX_QUAD_VERTS]));
      GL_CHECK(
        glBufferData(
          GL_ARRAY_BUFFER,
          sizeof(quad_vertices),
          quad_vertices,
          GL_STATIC_DRAW
        )
      );

      // Instantiate vertex attributes buffer.
      GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_IDX_VERTEX_ATTRIB]));
      GL_CHECK(
        glBufferData(
          GL_ARRAY_BUFFER,
          sizeof(vertex_attributes),
          nullptr,
          GL_DYNAMIC_DRAW
        )
      );

      // Setup the tile texture array.
      GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_TILESETS]));
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
          2048,
          2048,
          64
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

    uintptr_t get_render_texture() {
      return textures[TEXTURE_IDX_RENDER];
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

    const TilesetHandle* set_map(uint16_t index) {
      map_index = index;
      return &map_tileset_handles[index];
    }

    const SpriteHandle* load_entities(
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
      SpriteHandle* handle = new SpriteHandle {
        .begin = begin,
        .count = entities.size(),
      };
      entity_handles.insert({handle, std::unique_ptr<SpriteHandle>(handle)});
      return handle;
    }

    void unload_entities(
      const std::vector<const SpriteHandle*>& handles
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

    void clear() {
      bind_framebuffer();
      GL_CHECK(glViewport(0, 0, 256, 240));
      GL_CHECK(glClearColor(0, 0, 0, 1));
      GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
      unbind_framebuffer();
    }

    void render_tiles(
      size_t start_layer_idx,
      ssize_t layer_count
    ) {
      if (layer_count <= 0) {
        return;
      }
      // Get tile count.
      auto map_size = static_cast<geometry::Vector<size_t>>(maps[map_index].size);
      auto map_area = map_size.x * map_size.y;
      auto tile_offset = map_area * start_layer_idx;
      auto tile_count = map_area * layer_count;
      // Get texture indices.
      uint8_t texture_indices[16];
      geometry::Vector<uint32_t> texture_sizes[16];
      auto texture = map_tile_textures[map_index].begin();
      for (int i = 0;
           texture != map_tile_textures[map_index].end();
           i++, texture++) {
        texture_indices[i] = (*texture)->index;
        texture_sizes[i] = (*texture)->size;
      }
      // Get layer parallax.
      geometry::Vector<float> layer_parallax[maps[map_index].layers.size()];
      for (int i = 0; i < maps[map_index].layers.size(); i++) {
        layer_parallax[i] = maps[map_index].layers[i].parallax;
      }
      // Populate vertex attributes.
      VertexAttributes* curr_attrs = vertex_attributes;
      size_t count = 0;
      for (size_t i = tile_offset; i < tile_offset + tile_count; i++) {
        if (count++ >= MAX_ATTRIBUTES) {
          break;
        }
        auto tile = maps[map_index].tiles[i];
        VertexAttributes attrs = {0};
        if (tile) {
          get_tile_quad_transform(attrs.quad_transform, i, layer_parallax);
          get_tile_texture_transform(
            attrs.texture_transform,
            tile,
            texture_sizes,
            texture_indices
          );
        }
        *curr_attrs++ = attrs;
      }
      // Draw arrays.
      draw_arrays(tile_count);
    }

    void render_sprites(
      const std::vector<const SpriteHandle*>& sprites,
      size_t layer_index
    ) {
      // Get sprite and tileset sizes
      geometry::Vector<uint32_t> texture_sizes[16];
      uint8_t texture_indices[64];
      for (auto& pair : sprite_textures) {
        texture_sizes[pair.first->index] = pair.first->size;
        texture_indices[pair.first->index] = pair.second;
      }
      // Get sprite count.
      size_t sprite_count = 0;
      for (const auto& handle : sprites) {
        sprite_count += handle->count;
      }
      if (sprite_count > MAX_ATTRIBUTES) {
        sprite_count = MAX_ATTRIBUTES;
      }
      // Populate the sprite attributes data.
      VertexAttributes* curr_attrs = vertex_attributes;
      size_t count = 0;
      for (const auto& handle : sprites) {
        auto it = handle->begin;
        for (int i = 0; i < handle->count; i++, it++) {
          if (count++ >= MAX_ATTRIBUTES) {
            break;
          }
          const auto& sprite = *it;
          VertexAttributes attrs;
          get_sprite_quad_transform(
            attrs.quad_transform,
            sprite.entity,
            layer_index,
            count,
            sprite_count
          );
          get_sprite_texture_transform(
            attrs.texture_transform,
            sprite.entity,
            texture_sizes[sprite.texture->index],
            texture_indices[sprite.texture->index]
          );
          *curr_attrs++ = attrs;
        }
      }
      // Draw arrays.
      draw_arrays(sprite_count);
    }

    geometry::Vector<float> camera_position;

  private:

    void draw_arrays(
      GLsizei count
    ) {
      bind_framebuffer();
      // Use the shader program.
      program->use();
      // Bind the tilesets texture.
      GL_CHECK(glActiveTexture(GL_TEXTURE0));
      GL_CHECK(glBindTexture(GL_TEXTURE_2D_ARRAY, textures[TEXTURE_IDX_TILESETS]));
      // Set the tilesets uniform.
      GL_CHECK(glUniform1i(uniform_locations.tilesets, 0));
      // Bind the quad vertex array.
      GL_CHECK(glBindVertexArray(vertex_arrays[VERTEX_ARRAY_IDX_QUAD_VERTS]));
      GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_IDX_QUAD_VERTS]));
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_VERTEX,
          3,
          GL_FLOAT,
          GL_FALSE,
          3 * sizeof(GLfloat),
          nullptr
        )
      );
      // Set up the vertex attributes.
      GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_IDX_VERTEX_ATTRIB]));
      GL_CHECK(
        glBufferSubData(
          GL_ARRAY_BUFFER,
          static_cast<GLintptr>(0),
          static_cast<GLsizeiptr>(count * sizeof(VertexAttributes)),
          vertex_attributes
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_QUAD_TRANSFORM + 0,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, quad_transform) + 0 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_QUAD_TRANSFORM + 1,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, quad_transform) + 4 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_QUAD_TRANSFORM + 2,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, quad_transform) + 8 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_QUAD_TRANSFORM + 3,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, quad_transform) + 12 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_TEXTURE_TRANSFORM + 0,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, texture_transform) + 0 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_TEXTURE_TRANSFORM + 1,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, texture_transform) + 4 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_TEXTURE_TRANSFORM + 2,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, texture_transform) + 8 * sizeof(GLfloat)
          )
        )
      );
      GL_CHECK(
        glVertexAttribPointer(
          ATTRIB_LOCATION_TEXTURE_TRANSFORM + 3,
          4,
          GL_FLOAT,
          GL_FALSE,
          sizeof(VertexAttributes),
          reinterpret_cast<void*>(
            offsetof(VertexAttributes, texture_transform) + 12 * sizeof(GLfloat)
          )
        )
      );
      // Set attribute divisors for instancing.
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_QUAD_TRANSFORM + 0, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_QUAD_TRANSFORM + 1, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_QUAD_TRANSFORM + 2, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_QUAD_TRANSFORM + 3, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 0, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 1, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 2, 1));
      GL_CHECK(glVertexAttribDivisor(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 3, 1));
      // Enable vertex attributes.
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_VERTEX));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_QUAD_TRANSFORM + 0));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_QUAD_TRANSFORM + 1));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_QUAD_TRANSFORM + 2));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_QUAD_TRANSFORM + 3));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 0));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 1));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 2));
      GL_CHECK(glEnableVertexAttribArray(ATTRIB_LOCATION_TEXTURE_TRANSFORM + 3));
      // Draw arrays.
      GL_CHECK(glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, count));
      GL_CHECK(glBindVertexArray(0));
      unbind_framebuffer();
    }

    void bind_framebuffer() {
      // Enable depth test
      GL_CHECK(glEnable(GL_DEPTH_TEST));
      GL_CHECK(glDepthFunc(GL_LESS));

      // Enable alpha blending.
      GL_CHECK(glEnable(GL_BLEND));
      GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

      // Render to framebuffer.
      GL_CHECK(
        glBindFramebuffer(
          GL_FRAMEBUFFER,
          framebuffers[FRAMEBUFFER_IDX_RENDER]
        )
      );
    }

    void get_tile_quad_transform(
      mat4 transform,
      size_t index,
      const geometry::Vector<GLfloat>* layer_parallax
    ) {
      auto map_size = static_cast<geometry::Vector<size_t>>(maps[map_index].size);
      auto map_area = map_size.x * map_size.y;
      uint32_t layer_index = index / map_area;
      auto layer_start = map_area * layer_index;
      auto layer_offset = index - layer_start;
      GLfloat layer_z = (15 - layer_index) / 16.f;
      mat4 scale, translate, view, quad;
      mat4_scale(scale, 16, 16, layer_z);
      mat4_translate(
        translate,
        16.f * (layer_offset % map_size.x),
        16.f * (layer_offset / map_size.x),
        0
      );
      mat4_translate(
        view,
        -camera_position.x * layer_parallax[layer_index].x,
        -camera_position.y * layer_parallax[layer_index].y,
        0
      );
      mat4_identity(quad);
      mat4_mult_mat4(quad, quad, scale);
      mat4_mult_mat4(quad, quad, translate);
      mat4_mult_mat4(quad, quad, view);
      clip(quad, quad);
      memcpy(transform, quad, sizeof(quad));
    }

    void get_tile_texture_transform(
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

    void get_sprite_quad_transform(
      mat4 transform,
      const Entity* entity,
      size_t layer_index,
      size_t sprite_index,
      size_t sprite_count
    ) {
      auto tile_size = entity->tileset.tile_size;
      auto sprite_z = (1 + sprite_count - sprite_index) / (1.f + sprite_count);
      auto layer_z = (15 - layer_index + sprite_z) / 16.f;
      auto map_position = maps[map_index].position;
      mat4 to, flip, from, scale, entity_transform, translate, view, quad;
      mat4_translate(to, -.5f, -.5f, 0);
      mat4_scale(
        flip,
        entity->attributes.flip_x ? -1 : 1,
        entity->attributes.flip_y ? -1 : 1,
        1
      );
      mat4_translate(from, .5f, .5f, 0);
      mat4_scale(scale, tile_size.x, tile_size.y, layer_z);
      mat4_from_mat3(entity_transform, &entity->transform[0]);
      mat4_translate(
        translate,
        entity->position.x,
        entity->position.y - tile_size.y,
        0
      );
      mat4_translate(
        view,
        -16.f * map_position.x - camera_position.x,
        -16.f * map_position.y - camera_position.y,
        0
      );
      mat4_identity(quad);
      mat4_mult_mat4(quad, quad, to);
      mat4_mult_mat4(quad, quad, flip);
      mat4_mult_mat4(quad, quad, from);
      mat4_mult_mat4(quad, quad, scale);
      mat4_mult_mat4(quad, quad, entity_transform);
      mat4_mult_mat4(quad, quad, translate);
      mat4_mult_mat4(quad, quad, view);
      clip(quad, quad);
      memcpy(transform, quad, sizeof(quad));
    }

    void get_sprite_texture_transform(
      mat4 transform,
      const Entity* entity,
      const geometry::Vector<uint32_t>& texture_size,
      uint8_t texture_index
    ) {
      auto tile_size = entity->tileset.tile_size;
      auto tile_index = entity->tile_index;
      auto pos = tile_index * tile_size;
      auto position = geometry::Vector<uint16_t>(
        pos.x % texture_size.x,
        (pos.x / texture_size.x) * tile_size.y
      );
      get_texture_transform(
        transform,
        position,
        tile_size,
        texture_size,
        texture_index
      );
    }

    void get_texture_transform(
      mat4 transform,
      const geometry::Vector<uint16_t>& position,
      const geometry::Vector<uint16_t>& tile_size,
      const geometry::Vector<uint32_t>& tileset_size,
      uint8_t texture_index
    ) {
      mat4 to, flip, from, scale, translate, texture;
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
      mat4_identity(texture);
      mat4_mult_mat4(texture, texture, to);
      mat4_mult_mat4(texture, texture, flip);
      mat4_mult_mat4(texture, texture, from);
      mat4_mult_mat4(texture, texture, scale);
      mat4_mult_mat4(texture, texture, translate);
      memcpy(transform, texture, sizeof(texture));
    }

    void clip(
      mat4 transform,
      const mat4 view
    ) {
      mat4 translate, scale, clip;
      mat4_translate(translate, -128, -120, 0);
      mat4_scale(scale, 1.f / 128, -1.f / 120, 1);
      mat4_identity(clip);
      mat4_mult_mat4(clip, clip, view);
      mat4_mult_mat4(clip, clip, translate);
      mat4_mult_mat4(clip, clip, scale);
      memcpy(transform, clip, sizeof(clip));
    }

    void unbind_framebuffer() {
      GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    }

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

    std::unique_ptr<Program> program;

    HandlesPointer<Framebuffers> framebuffers;

    HandlesPointer<Renderbuffers> renderbuffers;

    HandlesPointer<Buffers> buffers;

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
      const SpriteHandle*,
      std::unique_ptr<SpriteHandle>
    > entity_handles;

    std::vector<std::list<Texture*>> map_tile_textures;

    std::vector<TilesetHandle> map_tileset_handles;

    TextureList::iterator world_textures;

    size_t world_textures_count;

    struct {
      GLint view;
      GLint tilesets;
    } uniform_locations;

    VertexAttributes vertex_attributes[MAX_ATTRIBUTES];

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

  uintptr_t get_render_texture() {
    return renderer->get_render_texture();
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

  const SpriteHandle* load_entities(
    const std::vector<const Entity*>& entities,
    const std::vector<const TilesetHandle*>& tilesets
  ) {
    return renderer->load_entities(entities, tilesets);
  }

  void unload_entities(const std::vector<const SpriteHandle*>& handles) {
    renderer->unload_entities(handles);
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

  void clear() {
    renderer->clear();
  }

  void render_tiles(size_t start_layer_idx, ssize_t layer_count) {
    renderer->render_tiles(start_layer_idx, layer_count);
  }

  void render_sprites(
    const std::vector<const SpriteHandle*>& entities,
    size_t layer_index
  ) {
    renderer->render_sprites(entities, layer_index);
  }

}
