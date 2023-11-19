#include <thread>
#include "renderer.h"

namespace ultra::renderer {

  uint32_t time;

  static auto render_target_duration =
    std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::seconds(1)
    ) / 60;

  static std::chrono::time_point<std::chrono::steady_clock> last_render_time;
  static std::chrono::time_point<std::chrono::steady_clock> last_sample_time;
  static std::chrono::time_point<std::chrono::steady_clock> curr_time;

  static unsigned frames_rendered = 0;

  void predraw() {
    // Declare render target duration.
    // Update timers.
    curr_time = std::chrono::steady_clock::now();
    if (last_render_time.time_since_epoch().count()) {
      std::chrono::microseconds time_since_last_render =
        std::chrono::duration_cast<std::chrono::microseconds>(
          curr_time - last_render_time
        );
      if (time_since_last_render < render_target_duration) {
        std::this_thread::sleep_for(
          render_target_duration - time_since_last_render
        );
      }
    }
  }

  // FPS sample target duration.
  static auto sample_target_duration =
    std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::seconds(3)
    );

  void postdraw() {
    // Check sample times.
    last_render_time = curr_time;
    frames_rendered++;
    if (!last_sample_time.time_since_epoch().count()) {
      last_sample_time = last_render_time;
    } else {
      std::chrono::microseconds time_since_last_sample =
        std::chrono::duration_cast<std::chrono::microseconds>(
          last_render_time - last_sample_time
        );
      if (time_since_last_sample >= sample_target_duration) {
        float usec = static_cast<float>(time_since_last_sample.count());
        float sec = usec / 1000000;
        //printf("%f\n", frames_rendered / sec);
        last_sample_time = last_render_time;
        frames_rendered = 0;
      }
    }
  }

}
