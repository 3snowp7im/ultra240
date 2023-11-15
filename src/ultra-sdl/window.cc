#include <glad/glad.h>
#include <list>
#include <memory>
#include <queue>
#include <SDL.h>
#include <SDL_opengl.h>
#include <set>
#include <stdexcept>
#include <ultra240/image.h>
#include <ultra240/window.h>
#include <unordered_map>
#include "shader/shader.h"

namespace ultra {

  namespace sdl {

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

  }

  struct Window::TilesetHandle {
    sdl::TextureList::iterator begin;
    size_t count;
    std::unordered_map<std::string, const sdl::Texture*> texture_map;
  };

  struct Window::EntityHandle {
    std::list<sdl::Sprite>::iterator begin;
    size_t count;
  };

}

// Number of data buffers.
#define BUFFERS_COUNT  3

// Array indexes for each data buffer.
enum {
  BUFFER_IDX_TILE,
  BUFFER_IDX_SPRITE,
  BUFFER_IDX_QUAD,
};

// Number of texture arrays.
#define TEXTURES_COUNT  3

// Array indices for each texture array.
enum {
  TEXTURE_IDX_TILE,
  TEXTURE_IDX_ANIMATION,
  TEXTURE_IDX_RENDERED,
};

#define MAX_SPRITES  512

namespace ultra::sdl {

  class WindowImpl : public Window::Impl {
  public:

    static SDL_Window* create_window(
      const char* title,
      SettingsManager& window_settings
    ) {
      bool updated = false;
      // Set some defaults if not present.
      if (!window_settings["fullscreen"].is_defined()) {
        window_settings["fullscreen"] = true;
        updated = true;
      }
      // Set default window position and flags.
      int x, y;
      int w, h;
      x = y = SDL_WINDOWPOS_CENTERED;
      w = h = 0;
      // Get stored window position and dimensions.
      if (window_settings["x"].is_defined()) {
        x = window_settings["x"].to_int();
      }
      if (window_settings["y"].is_defined()) {
        y = window_settings["y"].to_int();
      }
      if (window_settings["w"].is_defined()) {
        w = window_settings["w"].to_int();
      }
      if (window_settings["h"].is_defined()) {
        h = window_settings["h"].to_int();
      }
      if (w == 0 || h == 0) {
        SDL_Rect bounds;
        if (SDL_GetDisplayBounds(0, &bounds) != 0) {
          throw std::runtime_error("Window: could not get display bounds");
        }
        w = bounds.w;
        h = bounds.h;
        window_settings["w"] = w;
        window_settings["h"] = h;
        updated = true;
      }
      Uint32 flags = SDL_WINDOW_OPENGL;
      if (window_settings["fullscreen"].to_bool()) {
        flags |= SDL_WINDOW_FULLSCREEN;
      }
      SDL_Window* window = SDL_CreateWindow(title, x, y, w, h, flags);
      if (window == nullptr) {
        throw std::runtime_error(
          "Window: could not create window: " + std::string(SDL_GetError())
        );
      }
      // Get actual window placement.
      int actual_x, actual_y;
      int actual_w, actual_h;
      SDL_GetWindowPosition(window, &actual_x, &actual_y);
      SDL_GetWindowSize(window, &actual_w, &actual_h);
      if (actual_x != x) {
        x = actual_x;
        window_settings["x"] = x;
        updated = true;
      }
      if (actual_y != y) {
        y = actual_y;
        window_settings["y"] = y;
        updated = true;
      }
      if (actual_w != w) {
        w = actual_w;
        window_settings["w"] = w;
        updated = true;
      }
      if (actual_h != h) {
        h = actual_h;
        window_settings["w"] = w;
        updated = true;
      }
      if (updated) {
        window_settings.save();
      }
      // TODO: when window is moved, update display setting.
      return window;
    }

    static SDL_Joystick* create_joystick(SettingsManager& controls_settings) {
      if (controls_settings["controller"].is_defined()) {
        auto controller = controls_settings["controller"].to_string();
        if (controller != "") {
          for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (controller == SDL_GameControllerNameForIndex(i)) {
              return SDL_JoystickOpen(i);
            }
          }
        }
      }
      return nullptr;
    }

    static SDL_GLContext create_gl_context(SDL_Window* window) {
      SDL_GLContext context = SDL_GL_CreateContext(window);
      if (context == nullptr) {
        throw std::runtime_error(
          "Window: could not create GL context: " + std::string(SDL_GetError())
        );
      }
      return context;
    }

    static void compile_shader(
      const char* name,
      GLuint shader,
      unsigned char* shader_source,
      unsigned int shader_source_len
    ) {
      glShaderSource(
        shader,
        1,
        reinterpret_cast<GLchar**>(&shader_source),
        reinterpret_cast<GLint*>(&shader_source_len)
      );
      glCompileShader(shader);
      GLint success;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
      if (!success) {
        GLint log_length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetShaderInfoLog(shader, log_length, nullptr, log);
        throw std::runtime_error(
          "Window: could not compile " + std::string(name) + ": " + log
        );
      }
    }

    static void link_program(
      const char* name,
      GLuint program
    ) {
      glLinkProgram(program);
      GLint success;
      glGetProgramiv(program, GL_LINK_STATUS, &success);
      if (!success) {
        GLint log_length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_length);
        GLchar log[log_length];
        glGetProgramInfoLog(program, log_length, nullptr, log);
        throw std::runtime_error(
          "Window: could not link " + std::string(name) + ": " + log
        );
      }
    }

    static key::Event key_event_from_sdl(
      const SDL_KeyboardEvent& sdl_event
    ) {
      key::Event e;
      e.code = sdl_event.keysym.scancode;
      e.key = static_cast<key::Symbol>(sdl_event.keysym.sym);
      e.mod = sdl_event.keysym.mod;
      e.repeat = sdl_event.repeat;
      return e;
    }

    static controller_hat::Event controller_hat_event_from_sdl(
      const SDL_JoyHatEvent& sdl_event
    ) {
      controller_hat::Event e;
      e.value = static_cast<controller_hat::Value>(sdl_event.value);
      return e;
    }

    static controller_button::Event controller_button_event_from_sdl(
      const SDL_JoyButtonEvent& sdl_event
    ) {
      controller_button::Event e;
      e.button = static_cast<controller_button::Button>(sdl_event.button);
      return e;
    }

    static WindowImpl* deref(std::unique_ptr<Window::Impl>& impl) {
      return reinterpret_cast<WindowImpl*>(impl.get());
    }

    WindowImpl(
      const char* title,
      SettingsManager& window_settings,
      SettingsManager& controls_settings
    ) : closed(false),
        maps(nullptr),
        sdl_window(create_window(title, window_settings), SDL_DestroyWindow),
        gl_context(create_gl_context(sdl_window.get())),
        joystick(create_joystick(controls_settings), SDL_JoystickClose),
        window_settings(window_settings),
        controls_settings(controls_settings),
        camera_position({0.f, 0.f}),
        world_textures_count(0),
        time(0) {

      // Calculate rendering viewport.
      const float aspect = 16.f / 15;
      SDL_GetWindowSize(sdl_window.get(), &size.x, &size.y);
      if (size.x > size.y) {
        view_size.x = static_cast<int>(size.y * aspect);
        view_size.y = size.y;
      } else {
        view_size.x = size.x;
        view_size.y = static_cast<int>(size.x / aspect);
      }

      // Set Vsync.
      SDL_GL_SetSwapInterval(1);

      // Load GL extensions.
      gladLoadGLLoader(reinterpret_cast<GLADloadproc>(SDL_GL_GetProcAddress));

      // Enable depth test
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS); 
      // Enable alpha blending.
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      // Compile shaders.
      GLuint tile_vert_shader = glCreateShader(GL_VERTEX_SHADER);
      if (!tile_vert_shader) {
        throw std::runtime_error("Window: could not create tile vertex shader");
      }
      compile_shader(
        "tile vertex shader",
        tile_vert_shader,
        shader_tile_vert_glsl,
        shader_tile_vert_glsl_len
      );
      GLuint sprite_vert_shader = glCreateShader(GL_VERTEX_SHADER);
      if (!sprite_vert_shader) {
        throw std::runtime_error(
          "Window: could not create sprite vertex shader"
        );
      }
      compile_shader(
        "sprite vertex shader",
        sprite_vert_shader,
        shader_sprite_vert_glsl,
        shader_sprite_vert_glsl_len
      );
      GLuint tex_geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
      if (!tex_geom_shader) {
        throw std::runtime_error(
          "Window: could not create texture geometry shader"
        );
      }
      compile_shader(
        "texture geometry shader",
        tex_geom_shader,
        shader_tex_geom_glsl,
        shader_tex_geom_glsl_len
      );
      GLuint line_geom_shader = glCreateShader(GL_GEOMETRY_SHADER);
      if (!line_geom_shader) {
        throw std::runtime_error(
          "Window: could not create line geometry shader"
        );
      }
      compile_shader(
        "line geometry shader",
        line_geom_shader,
        shader_line_geom_glsl,
        shader_line_geom_glsl_len
      );
      GLuint tex_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
      if (!tex_frag_shader) {
        throw std::runtime_error(
          "Window: could not create texture fragment shader"
        );
      }
      compile_shader(
        "texture fragment shader",
        tex_frag_shader,
        shader_tex_frag_glsl,
        shader_tex_frag_glsl_len
      );
      GLuint line_frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
      if (!line_frag_shader) {
        throw std::runtime_error(
          "Window: could not create line fragment shader"
        );
      }
      compile_shader(
        "line fragment shader",
        line_frag_shader,
        shader_line_frag_glsl,
        shader_line_frag_glsl_len
      );
      GLuint passthrough_shader = glCreateShader(GL_VERTEX_SHADER);
      if (!passthrough_shader) {
        throw std::runtime_error("Window: could not create passthrough shader");
      }
      compile_shader(
        "passthrough shader",
        passthrough_shader,
        shader_passthrough_glsl,
        shader_passthrough_glsl_len
      );
      GLuint quad_shader = glCreateShader(GL_FRAGMENT_SHADER);
      if (!quad_shader) {
        throw std::runtime_error("Window: could not create quad shader");
      }
      compile_shader(
        "quad shader",
        quad_shader,
        shader_quad_glsl,
        shader_quad_glsl_len
      );

      // Link tile shader program.
      tile_program = glCreateProgram();
      if (!tile_program) {
        throw std::runtime_error("Window: could not create tile program");
      }
      glAttachShader(tile_program, tile_vert_shader);
      glAttachShader(tile_program, tex_geom_shader);
      glAttachShader(tile_program, tex_frag_shader);
      link_program("tile program", tile_program);

      // Link sprite shader program.
      sprite_program = glCreateProgram();
      if (!sprite_program) {
        throw std::runtime_error("Window: could not create sprite program");
      }
      glAttachShader(sprite_program, sprite_vert_shader);
      glAttachShader(sprite_program, tex_geom_shader);
      glAttachShader(sprite_program, tex_frag_shader);
      link_program("sprite program", sprite_program);

      // Link quad shader program.
      quad_program = glCreateProgram();
      if (!quad_program) {
        throw std::runtime_error("Window: could not create quad program");
      }
      glAttachShader(quad_program, passthrough_shader);
      glAttachShader(quad_program, quad_shader);
      link_program("quad program", quad_program);

      // Delete shaders.
      glDeleteShader(tile_vert_shader);
      glDeleteShader(sprite_vert_shader);
      glDeleteShader(tex_geom_shader);
      glDeleteShader(tex_frag_shader);
      glDeleteShader(line_geom_shader);
      glDeleteShader(line_frag_shader);
      glDeleteShader(passthrough_shader);
      glDeleteShader(quad_shader);

      // Generate textures.
      glGenTextures(TEXTURES_COUNT, textures);

      // Create frame buffer.
      glGenFramebuffers(1, &frame_buffer);
      glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
      // Create depth buffer.
      GLuint depth_buffer;
      glGenRenderbuffers(1, &depth_buffer);
      glBindRenderbuffer(GL_RENDERBUFFER, depth_buffer);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, 256, 240);
      glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER,
        depth_buffer
      );
      // Create render texture.
      glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_IDX_RENDERED]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        256,
        240,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
      );
      // Set rendered texture as color attachment 0.
      glFramebufferTexture(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        textures[TEXTURE_IDX_RENDERED],
        0
      );
      GLenum draw_buffer = GL_COLOR_ATTACHMENT0;
      glDrawBuffers(1, &draw_buffer);
      if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Window: could not attach frame buffer");
      }

      // Get tile uniform locations.
      for (int i = 0; i < 16; i++) {
        uniform_locations.tile.tileset_indices[i] = glGetUniformLocation(
          tile_program,
          ("tileset_indices[" + std::to_string(i) + "]").c_str()
        );
        uniform_locations.tile.tileset_sizes[i] = glGetUniformLocation(
          tile_program,
          ("tileset_sizes[" + std::to_string(i) + "]").c_str()
        );
        uniform_locations.tile.layer_parallax[i] = glGetUniformLocation(
          tile_program,
          ("layer_parallax[" + std::to_string(i) + "]").c_str()
        );
      }
      uniform_locations.tile.camera_position = glGetUniformLocation(
        tile_program,
        "camera_position"
      );
      uniform_locations.tile.map_size = glGetUniformLocation(
        tile_program,
        "map_size"
      );
      uniform_locations.tile.time = glGetUniformLocation(
        tile_program,
        "time"
      );
      uniform_locations.tile.animations = glGetUniformLocation(
        tile_program,
        "animations"
      );
      uniform_locations.tile.tilesets = glGetUniformLocation(
        tile_program,
        "tilesets"
      );

      // Get sprite uniform locations.
      for (int i = 0; i < 48; i++) {
        uniform_locations.sprite.tileset_indices[i] = glGetUniformLocation(
          sprite_program,
          ("tileset_indices[" + std::to_string(i) + "]").c_str()
        );
        uniform_locations.sprite.tile_sizes[i] = glGetUniformLocation(
          sprite_program,
          ("tile_sizes[" + std::to_string(i) + "]").c_str()
        );
        uniform_locations.sprite.tileset_sizes[i] = glGetUniformLocation(
          sprite_program,
          ("tileset_sizes[" + std::to_string(i) + "]").c_str()
        );
      }
      uniform_locations.sprite.camera_position = glGetUniformLocation(
        sprite_program,
        "camera_position"
      );
      uniform_locations.sprite.map_position = glGetUniformLocation(
        sprite_program,
        "map_position"
      );
      uniform_locations.sprite.layer_index = glGetUniformLocation(
        sprite_program,
        "layer_index"
      );
      uniform_locations.sprite.sprite_count = glGetUniformLocation(
        sprite_program,
        "sprite_count"
      );
      uniform_locations.sprite.tilesets = glGetUniformLocation(
        sprite_program,
        "tilesets"
      );

      // Get quad uniform locations.
      uniform_locations.quad.rendered_texture = glGetUniformLocation(
        quad_program,
        "rendered_texture"
      );

      // Generate array buffers.
      glGenBuffers(BUFFERS_COUNT, array_buffers);
      // Generate vertex array.
      glGenVertexArrays(BUFFERS_COUNT, vertex_arrays);
      // Instantiate sprite buffer.
      glBindBuffer(GL_ARRAY_BUFFER, array_buffers[BUFFER_IDX_SPRITE]);
      glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(sprite_data),
        nullptr,
        GL_DYNAMIC_DRAW
      );

      // Instantiate quad elements.
      GLuint quad_element_buffer;
      glGenBuffers(1, &quad_element_buffer);
      glBindVertexArray(vertex_arrays[BUFFER_IDX_QUAD]);
      GLfloat vertices[] = {
        -1., -1.,
        1., -1.,
        -1.,  1.,
        1.,  1.,
      };
      glBindBuffer(GL_ARRAY_BUFFER, array_buffers[BUFFER_IDX_QUAD]);
      glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
      GLuint indices[] = {0, 1, 2, 3};
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, quad_element_buffer);
      glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(indices),
        indices,
        GL_STATIC_DRAW
      );
      glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        2 * sizeof(GLfloat),
        reinterpret_cast<void*>(0)
      );
      glEnableVertexAttribArray(0);
      glBindVertexArray(0);

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

    ~WindowImpl() {
      SDL_GL_DeleteContext(gl_context);
    }

    void close() {
      sdl_window = nullptr;
      closed = true;
    }

    bool should_quit() {
      return closed;
    }

    bool poll_events(Event& e) {
      SDL_Event sdl_event;
      while (SDL_PollEvent(&sdl_event)) {
        switch (sdl_event.type) {
        case SDL_QUIT:
          e.type = Event::Type::Quit;
          break;
        case SDL_KEYDOWN:
          e.type = Event::Type::KeyDown;
          e.key = key_event_from_sdl(sdl_event.key);
          break;
        case SDL_KEYUP:
          e.type = Event::Type::KeyUp;
          e.key = key_event_from_sdl(sdl_event.key);
          break;
        case SDL_JOYAXISMOTION:
          // TODO
          break;
        case SDL_JOYHATMOTION:
          e.type = Event::Type::ControllerHatMotion;
          e.controller_hat
            = controller_hat_event_from_sdl(sdl_event.jhat);
          break;
        case SDL_JOYBUTTONDOWN:
          e.type = Event::Type::ControllerButtonDown;
          e.controller_button
            = controller_button_event_from_sdl(sdl_event.jbutton);
          break;
        case SDL_JOYBUTTONUP:
          e.type = Event::Type::ControllerButtonUp;
          e.controller_button
            = controller_button_event_from_sdl(sdl_event.jbutton);
          break;
        default:
          continue;
        }
        return true;
      }
      return false;
    }

    void set_camera_position(const geometry::Vector<float>& position) {
      camera_position = position;
    }

    const Window::TilesetHandle* load_tilesets(
      PathManager& pm,
      const std::vector<const Tileset*>& tilesets
    ) {
      auto begin = add_tilesets(
        pm,
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
      Window::TilesetHandle* handle = new Window::TilesetHandle {
        .begin = begin,
        .count = tilesets.size(),
        .texture_map = texture_map,
      };
      tileset_handles.insert({
        handle,
        std::unique_ptr<Window::TilesetHandle>(handle),
      });
      return handle;
    }

    void unload_tilesets(
      const std::vector<const Window::TilesetHandle*>& handles
    ) {
      for (const auto handle : handles) {
        remove_textures(handle->begin, handle->count);
        tileset_handles.erase(handle);
      }
    }

    void load_world(PathManager& pm, const World& world) {
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
          pm,
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
          pm,
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

    const Window::TilesetHandle* load_map(uint16_t index) {
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

    const Window::EntityHandle* load_entities(
      const std::vector<const Entity*>& entities,
      const std::vector<const Window::TilesetHandle*>& tilesets
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
      Window::EntityHandle* handle = new Window::EntityHandle {
        .begin = begin,
        .count = entities.size(),
      };
      entity_handles.insert({
        handle,
        std::unique_ptr<Window::EntityHandle>(handle),
      });
      return handle;
    }

    void unload_entities(
      const std::vector<const Window::EntityHandle*>& handles
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

    void* get_context() {
      return reinterpret_cast<void*>(frame_buffer);
    }

    void clear_context(float r, float g, float b, float a) {
      glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);
      glViewport(0, 0, 256, 240);
      glClearColor(r, g, b, a);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void render() {
      // Render to frame buffer.
      glBindFramebuffer(GL_FRAMEBUFFER, frame_buffer);

      // Render background tiles.
      render_tiles(0, maps[map_index].entities_index);

      // Use the sprite program.
      glUseProgram(sprite_program);
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

    void draw_context() {
      // Draw the quad.
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(
        (size.x - view_size.x) / 2,
        (size.y - view_size.y) / 2,
        view_size.x,
        view_size.y
      );
      glClearColor(0., 0., 0., 1.);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      glUseProgram(quad_program);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, textures[TEXTURE_IDX_RENDERED]);
      glUniform1i(uniform_locations.quad.rendered_texture, 0);
      glBindVertexArray(vertex_arrays[BUFFER_IDX_QUAD]);
      glDrawElements(
        GL_TRIANGLE_STRIP,
        4,
        GL_UNSIGNED_INT,
        reinterpret_cast<void*>(0)
      );
      // Update window.
      SDL_GL_SwapWindow(sdl_window.get());
    }

    void render_tiles(size_t start_layer_idx, ssize_t layer_count) {
      if (layer_count <= 0) {
        return;
      }
      // Use the tile program;
      glUseProgram(tile_program);
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

    TextureList::iterator add_tilesets(
      PathManager& pm,
      const Tileset* const* tilesets,
      size_t tilesets_count,
      Texture::Type type
    ) {
      auto begin = texture_list.insert(texture_list.end(), tilesets_count, {});
      auto texture = begin;
      for (int i = 0; i < tilesets_count; i++) {
        Image image = Image(pm, tilesets[i]->source.c_str());
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

    void remove_textures(TextureList::iterator begin, size_t count) {
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

    struct SpriteAttributes {
      float position[2];
      uint8_t index;
      uint16_t tile_index;
      uint8_t texture_coords_tl[2];
      uint8_t texture_coords_tr[2];
      uint8_t texture_coords_bl[2];
      uint8_t texture_coords_br[2];
    };

    std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> sdl_window;

    std::unique_ptr<SDL_Joystick, decltype(&SDL_JoystickClose)> joystick;

    SDL_GLContext gl_context;

    SettingsManager& window_settings;

    SettingsManager& controls_settings;

    bool closed;

    geometry::Vector<int> size;

    geometry::Vector<int> view_size;

    GLuint frame_buffer;

    GLuint tile_program;

    GLuint sprite_program;

    GLuint quad_program;

    GLuint array_buffers[BUFFERS_COUNT];

    GLuint vertex_arrays[BUFFERS_COUNT];

    GLuint textures[TEXTURES_COUNT];

    TextureList texture_list;

    std::list<sdl::Sprite> sprites;

    std::queue<uint8_t> texture_indices;

    std::unordered_map<const Texture*, uint8_t> sprite_textures;

    std::queue<uint8_t> sprite_indices;

    std::unordered_map<
      const Window::TilesetHandle*,
      std::unique_ptr<Window::TilesetHandle>
    > tileset_handles;

    std::unordered_map<
      const Window::EntityHandle*,
      std::unique_ptr<Window::EntityHandle>
    > entity_handles;

    std::vector<std::list<Texture*>> map_tile_textures;

    std::vector<Window::TilesetHandle> map_tileset_handles;

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

      struct {
        GLint rendered_texture;
      } quad;

    } uniform_locations;

    SpriteAttributes sprite_data[MAX_SPRITES];

    const World::Map* maps;

    size_t map_index;

    geometry::Vector<float> camera_position;

  };

}

namespace ultra {

  using namespace ultra::sdl;

  void Window::init() {
    // Initialize SDL.
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
      throw std::runtime_error(
        std::string("Window: could not initialize SDL: ") + SDL_GetError()
      );
    }
    // Use OpenGL 3.3 core.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(
      SDL_GL_CONTEXT_PROFILE_MASK,
      SDL_GL_CONTEXT_PROFILE_CORE
    );
    // Initialize text input.
    SDL_StartTextInput();
  }

  void Window::quit() {
    SDL_StopTextInput();
    SDL_Quit();
  }

  Window::Window(
    const char* title,
    SettingsManager& window_settings,
    SettingsManager& controls_settings
  ) : impl(new WindowImpl(title, window_settings, controls_settings)) {}

  Window::~Window() {}

  void Window::close() {
    auto impl = WindowImpl::deref(this->impl);
    impl->close();
  }

  bool Window::should_quit() {
    auto impl = WindowImpl::deref(this->impl);
    return impl->should_quit();
  }

  bool Window::poll_events(Event& event) {
    auto impl = WindowImpl::deref(this->impl);
    return impl->poll_events(event);
  }

  void Window::set_camera_position(const geometry::Vector<float>& position) {
    auto impl = WindowImpl::deref(this->impl);
    impl->set_camera_position(position);
  }

  const Window::TilesetHandle* Window::load_tilesets(
    PathManager& pm,
    const std::vector<const Tileset*>& tilesets
  ) {
    auto impl = WindowImpl::deref(this->impl);
    return impl->load_tilesets(pm, tilesets);
  }

  void Window::unload_tilesets(
    const std::vector<const Window::TilesetHandle*>& handles
  ) {
    auto impl = WindowImpl::deref(this->impl);
    impl->unload_tilesets(handles);
  }

  void Window::load_world(PathManager& pm, const World& world) {
    auto impl = WindowImpl::deref(this->impl);
    impl->load_world(pm, world);
  }

  void Window::unload_world() {
    auto impl = WindowImpl::deref(this->impl);
    impl->unload_world();
  }

  const Window::TilesetHandle* Window::load_map(uint16_t index) {
    auto impl = WindowImpl::deref(this->impl);
    return impl->load_map(index);
  }

  const Window::EntityHandle* Window::load_entities(
    const std::vector<const Entity*>& entities,
    const std::vector<const TilesetHandle*>& tilesets
  ) {
    auto impl = WindowImpl::deref(this->impl);
    return impl->load_entities(entities, tilesets);
  }

  void Window::unload_entities(const std::vector<const EntityHandle*>& handles) {
    auto impl = WindowImpl::deref(this->impl);
    impl->unload_entities(handles);
  }

  void* Window::get_context() {
    auto impl = WindowImpl::deref(this->impl);
    return impl->get_context();
  }

  void Window::clear_context(float r, float g, float b, float a) {
    auto impl = WindowImpl::deref(this->impl);
    impl->clear_context(r, g, b, a);
  }

  void Window::render() {
    auto impl = WindowImpl::deref(this->impl);
    predraw();
    impl->render();
    postdraw();
    impl->time++;
  }

  void Window::draw_context() {
    auto impl = WindowImpl::deref(this->impl);
    impl->draw_context();
  }

}
