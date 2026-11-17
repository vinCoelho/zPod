// zPod application: player logic, pet behaviour and screen drawing.
// Platform independent; talks to the device through Hal.
#pragma once
#include <LovyanGFX.hpp>

#include "hal.h"
#include "pet.h"

namespace zpod {

class App {
 public:
  explicit App(Hal& hal) : hal_(hal) {}

  void begin();                                 // power-on: load state, pet says hello
  void onButton(Button b, bool longPress);
  void tick(float dt);
  void render(lgfx::LovyanGFX& g);              // draws one full 240x320 frame
  bool screenOn() const { return screenOn_; }
  Pet& pet() { return pet_; }

 private:
  enum class Phase : uint8_t { Running, FallingAsleep, PoweringOff, Off };

  void togglePlay();
  void startTrack();
  void changeTrack(int delta, bool hop);
  void changeFolder(int delta);
  void changeVolume(int delta);
  void beginPowerOff();
  void wakeScreen();
  void save();

  void drawScene(lgfx::LovyanGFX& g);
  void drawStatusIcons(lgfx::LovyanGFX& g);
  void drawVolume(lgfx::LovyanGFX& g);
  void drawStrip(lgfx::LovyanGFX& g);

  Hal& hal_;
  Pet pet_;
  Phase phase_ = Phase::Running;
  float phaseTime_ = 0;
  float idle_ = 0;
  float time_ = 0;
  float volumeOverlay_ = 0;
  float batteryPoll_ = 0;
  float titleScroll_ = 0;
  int folder_ = 0, track_ = 0, volume_ = 0;
  int battery_ = 100;
  bool hasMusic_ = false;
  bool loaded_ = false;     // current track is open in the player
  bool playing_ = false;
  bool screenOn_ = true;
};

}  // namespace zpod
