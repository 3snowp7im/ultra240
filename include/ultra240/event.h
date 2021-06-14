#pragma once

#include <ultra240/controller_button.h>
#include <ultra240/controller_hat.h>
#include <ultra240/key.h>

namespace ultra {

  struct Event {

    enum Type {
      Quit,
      KeyDown,
      KeyUp,
      ControllerHatMotion,
      ControllerButtonDown,
      ControllerButtonUp,
    } type;

    union {
      key::Event key;
      controller_button::Event controller_button;
      controller_hat::Event controller_hat;
    };
  };

}
