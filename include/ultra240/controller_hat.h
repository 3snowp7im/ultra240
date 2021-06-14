#pragma once

namespace ultra::controller_hat {

  enum Value {
    Center = 0,
    Up = 1,
    Right = 2,
    RightUp = 3,
    Down = 4,
    RightDown = 6,
    Left = 8,
    LeftUp = 9,
    LeftDown = 12,
  };

  struct Event {
    Value value;
  };

}
