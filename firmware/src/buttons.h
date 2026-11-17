// Turns raw button levels into press / long-press / repeat events.
#pragma once
#include "config.h"
#include "hal.h"

namespace zpod {

class Buttons {
 public:
  // Call every loop with the debounced-enough raw state. `emit(button, longPress)` fires:
  //  - volume: on press, then repeating while held
  //  - touch: on press
  //  - others: long press once the threshold is reached, short press on release
  template <typename Emit>
  void update(Button b, bool pressed, uint32_t nowMs, Emit&& emit) {
    State& s = state_[static_cast<int>(b)];
    const bool repeats = (b == Button::VolUp || b == Button::VolDown);
    const bool immediate = repeats || b == Button::Touch;
    if (pressed && !s.down) {
      if (nowMs - s.changedMs < 30) return;   // debounce
      s.down = true;
      s.changedMs = s.lastFireMs = nowMs;
      s.longFired = false;
      if (immediate) emit(b, false);
    } else if (pressed && s.down) {
      if (repeats && nowMs - s.lastFireMs >= kRepeatMs && nowMs - s.changedMs >= 500) {
        s.lastFireMs = nowMs;
        emit(b, false);
      } else if (!immediate && !s.longFired && nowMs - s.changedMs >= kLongPressMs) {
        s.longFired = true;
        emit(b, true);
      }
    } else if (!pressed && s.down) {
      if (nowMs - s.changedMs < 30) return;
      s.down = false;
      s.changedMs = nowMs;
      if (!immediate && !s.longFired) emit(b, false);
    }
  }

 private:
  struct State {
    bool down = false, longFired = false;
    uint32_t changedMs = 0, lastFireMs = 0;
  };
  State state_[static_cast<int>(Button::Count)];
};

}  // namespace zpod
