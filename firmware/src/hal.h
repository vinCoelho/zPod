// Hardware abstraction: everything the app needs from the device.
// Implemented by main_esp32.cpp (real hardware) and main_sim.cpp (desktop simulator).
#pragma once
#include <stdint.h>

namespace zpod {

enum class Button : uint8_t { Play, Prev, Next, VolUp, VolDown, Touch, Count };

struct Saved {
  int folder = 0;
  int track = 0;
  int volume = -1;   // -1 = nothing saved yet
};

struct Hal {
  virtual ~Hal() = default;

  // Music library: each folder on the card is a playlist.
  virtual int folderCount() = 0;
  virtual int trackCount(int folder) = 0;
  virtual const char* folderName(int folder) = 0;
  virtual const char* trackTitle(int folder, int track) = 0;

  // Player.
  virtual bool play(int folder, int track) = 0;   // starts from the beginning, unpaused
  virtual void setPaused(bool paused) = 0;
  virtual void stop() = 0;
  virtual bool finished() = 0;                    // current track reached its end
  virtual int positionSec() = 0;
  virtual int durationSec() = 0;
  virtual void setVolume(int step) = 0;           // 0..kVolumeSteps

  // Power.
  virtual void setScreen(bool on) = 0;
  virtual int batteryPercent() = 0;
  virtual bool headphones() = 0;
  virtual void powerOff() = 0;                    // deep sleep; does not return on the device

  // Persistence.
  virtual void load(Saved& s) = 0;
  virtual void save(const Saved& s) = 0;
};

}  // namespace zpod
