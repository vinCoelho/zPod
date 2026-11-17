// The zPod pet: a little blob monster drawn with vector shapes.
#pragma once
#include <LovyanGFX.hpp>

namespace zpod {

enum class Mood : uint8_t { Sleep, Idle, Dance };
enum class Reaction : uint8_t {
  None, Hello, Bye, HopLeft, HopRight, BigEyes, Shrink, Startle, Tickle, Yawn
};

class Pet {
 public:
  void setMood(Mood m) { mood_ = m; }
  Mood mood() const { return mood_; }
  void react(Reaction r);
  bool reacting() const { return reaction_ != Reaction::None; }
  void setTired(bool tired) { tired_ = tired; }
  void setColor(uint32_t rgb) { color_ = rgb; }

  void update(float dt);
  // Draws the pet standing on `groundY`, horizontally centred on `cx`.
  void draw(lgfx::LovyanGFX& g, int cx, int groundY, uint32_t shadowColor) const;

 private:
  enum class Mouth : uint8_t { Smile, Open, Small, Flat, Yawn };
  struct Pose {
    float dx = 0, lift = 0;        // horizontal offset, height above ground
    float sx = 1, sy = 1;          // body squash and stretch
    float eyeOpen = 1, eyeScale = 1;
    float px = 0, py = 0;          // pupil offset
    float wave = -1;               // >= 0: one arm raised, value = wave phase
    float mouthSize = 1;
    bool happyEyes = false;
    Mouth mouth = Mouth::Smile;
  };
  Pose pose() const;

  Mood mood_ = Mood::Idle;
  Reaction reaction_ = Reaction::None;
  float reactionTime_ = 0;
  float t_ = 0;
  float blink_ = 2.5f;
  bool tired_ = false;
  uint32_t color_ = 0x8A6BFFu;
};

}  // namespace zpod
