#include "app.h"

#include <math.h>
#include <stdio.h>

#include "config.h"

namespace zpod {
namespace {

constexpr int kStripY = 232;          // pet scene above, song info below
constexpr int kGroundY = 204;
const lgfx::IFont* const kTitleFont = &fonts::lgfxJapanGothicP_20;
const lgfx::IFont* const kSmallFont = &fonts::lgfxJapanGothicP_16;

int wrap(int v, int n) { return n <= 0 ? 0 : ((v % n) + n) % n; }

}  // namespace

// ---------------------------------------------------------------- lifecycle

void App::begin() {
  Saved s;
  hal_.load(s);
  hasMusic_ = hal_.folderCount() > 0;
  folder_ = hasMusic_ ? wrap(s.folder, hal_.folderCount()) : 0;
  track_ = hasMusic_ ? wrap(s.track, hal_.trackCount(folder_)) : 0;
  volume_ = s.volume < 0 ? kVolumeDefault : s.volume;
  if (volume_ > kVolumeSteps) volume_ = kVolumeSteps;
  hal_.setVolume(volume_);

  phase_ = Phase::Running;
  phaseTime_ = idle_ = volumeOverlay_ = titleScroll_ = 0;
  loaded_ = playing_ = false;
  battery_ = hal_.batteryPercent();
  pet_.setTired(battery_ < kBatteryLowPercent);
  pet_.setColor(kPetColor);
  pet_.setMood(Mood::Idle);
  pet_.react(Reaction::Hello);
  screenOn_ = true;
  hal_.setScreen(true);
}

void App::save() {
  Saved s;
  s.folder = folder_;
  s.track = track_;
  s.volume = volume_;
  hal_.save(s);
}

void App::wakeScreen() {
  if (!screenOn_) {
    screenOn_ = true;
    hal_.setScreen(true);
  }
}

void App::beginPowerOff() {
  if (playing_) hal_.setPaused(true);
  playing_ = false;
  save();
  wakeScreen();
  pet_.setMood(Mood::Idle);
  pet_.react(Reaction::Bye);
  phase_ = Phase::PoweringOff;
  phaseTime_ = 0;
}

// ------------------------------------------------------------------- input

void App::onButton(Button b, bool longPress) {
  idle_ = 0;
  if (phase_ == Phase::Off) {
    // On the device powerOff() never returns; this only happens in the simulator.
    if (b == Button::Play || b == Button::Touch) begin();
    return;
  }
  if (phase_ == Phase::PoweringOff) return;

  const bool wasOff = !screenOn_;
  wakeScreen();

  if (phase_ == Phase::FallingAsleep) {       // any input wakes the pet up
    phase_ = Phase::Running;
    pet_.setMood(Mood::Idle);
    pet_.react(Reaction::Startle);
    return;
  }

  switch (b) {
    case Button::Touch:
      pet_.react(wasOff ? Reaction::Startle : Reaction::Tickle);
      break;
    case Button::Play:
      if (longPress) beginPowerOff();
      else togglePlay();
      break;
    case Button::Next:
      if (longPress) changeFolder(+1);
      else changeTrack(+1, true);
      break;
    case Button::Prev:
      if (longPress) changeFolder(-1);
      else changeTrack(-1, true);
      break;
    case Button::VolUp: changeVolume(+1); break;
    case Button::VolDown: changeVolume(-1); break;
    default: break;
  }
}

void App::togglePlay() {
  if (!hasMusic_) {
    pet_.react(Reaction::Shrink);
    return;
  }
  if (playing_) {
    hal_.setPaused(true);
    playing_ = false;
    pet_.setMood(Mood::Idle);
    save();
  } else if (loaded_) {
    hal_.setPaused(false);
    playing_ = true;
    pet_.setMood(Mood::Dance);
  } else {
    startTrack();
  }
}

void App::startTrack() {
  loaded_ = hal_.play(folder_, track_);
  playing_ = loaded_;
  pet_.setMood(playing_ ? Mood::Dance : Mood::Idle);
  titleScroll_ = 0;
  save();
}

void App::changeTrack(int delta, bool hop) {
  if (!hasMusic_) return;
  // "Previous" a few seconds into a song restarts it, like every player.
  if (!(delta < 0 && loaded_ && hal_.positionSec() > 3))
    track_ = wrap(track_ + delta, hal_.trackCount(folder_));
  if (hop) pet_.react(delta > 0 ? Reaction::HopRight : Reaction::HopLeft);
  titleScroll_ = 0;
  if (playing_) {
    startTrack();
  } else {
    hal_.stop();
    loaded_ = false;
    save();
  }
}

void App::changeFolder(int delta) {
  if (!hasMusic_) return;
  folder_ = wrap(folder_ + delta, hal_.folderCount());
  track_ = 0;
  pet_.react(Reaction::Hello);
  titleScroll_ = 0;
  if (playing_) {
    startTrack();
  } else {
    hal_.stop();
    loaded_ = false;
    save();
  }
}

void App::changeVolume(int delta) {
  int v = volume_ + delta;
  if (v < 0) v = 0;
  if (v > kVolumeSteps) v = kVolumeSteps;
  if (v != volume_) {
    volume_ = v;
    hal_.setVolume(volume_);
    pet_.react(delta > 0 ? Reaction::BigEyes : Reaction::Shrink);
  }
  volumeOverlay_ = 1.5f;
}

// -------------------------------------------------------------------- tick

void App::tick(float dt) {
  time_ += dt;
  if (phase_ == Phase::Off) return;
  pet_.update(dt);
  idle_ += dt;
  if (volumeOverlay_ > 0) volumeOverlay_ -= dt;
  titleScroll_ += dt;

  batteryPoll_ -= dt;
  if (batteryPoll_ <= 0) {
    batteryPoll_ = 5;
    battery_ = hal_.batteryPercent();
    pet_.setTired(battery_ < kBatteryLowPercent);
    if (battery_ < kBatteryEmptyPercent && phase_ == Phase::Running) beginPowerOff();
  }

  switch (phase_) {
    case Phase::Running:
      if (playing_) {
        if (hal_.finished()) changeTrack(+1, screenOn_);
        if (screenOn_ && idle_ > kScreenTimeoutPlaying) {
          screenOn_ = false;
          hal_.setScreen(false);
        }
      } else if (idle_ > kSleepTimeoutPaused) {
        pet_.react(Reaction::Yawn);
        phase_ = Phase::FallingAsleep;
        phaseTime_ = 0;
      }
      break;
    case Phase::FallingAsleep:
      phaseTime_ += dt;
      if (!pet_.reacting()) pet_.setMood(Mood::Sleep);
      if (phaseTime_ > 2.0f + kSleepToPowerOff) {
        save();
        phase_ = Phase::Off;
        screenOn_ = false;
        hal_.setScreen(false);
        hal_.powerOff();
      }
      break;
    case Phase::PoweringOff:
      phaseTime_ += dt;
      if (!pet_.reacting()) pet_.setMood(Mood::Sleep);
      if (phaseTime_ > 3.0f) {
        hal_.stop();
        phase_ = Phase::Off;
        screenOn_ = false;
        hal_.setScreen(false);
        hal_.powerOff();
      }
      break;
    case Phase::Off: break;
  }
}

// ------------------------------------------------------------------ render

void App::render(lgfx::LovyanGFX& g) {
  drawScene(g);
  pet_.draw(g, kScreenW / 2, kGroundY, pet_.mood() == Mood::Sleep ? 0x1E2947u : 0x6BBF59u);
  drawStatusIcons(g);
  if (volumeOverlay_ > 0) drawVolume(g);
  drawStrip(g);
}

void App::drawScene(lgfx::LovyanGFX& g) {
  const bool night = pet_.mood() == Mood::Sleep;
  g.fillRect(0, 0, kScreenW, kStripY, night ? 0x1B2040u : 0xBFE9FFu);
  if (night) {
    static const int16_t stars[][2] = {{24, 30}, {70, 64}, {110, 22}, {150, 52}, {196, 80},
                                       {40, 104}, {214, 26}, {176, 120}, {16, 150}};
    int i = 0;
    for (auto& s : stars) {
      float tw = 0.5f + 0.5f * sinf(time_ * 2 + i++);
      g.fillSmoothCircle(s[0], s[1], tw > 0.6f ? 2 : 1, 0xFFF4B8u);
    }
    g.fillSmoothCircle(44, 48, 17, 0xFFF4B8u);            // moon
    g.fillSmoothCircle(52, 42, 15, 0x1B2040u);
  } else {
    for (int i = 0; i < 2; ++i) {                          // clouds drifting by
      float cx = fmodf(time_ * (6 + 3 * i) + 150 * i, kScreenW + 120) - 60;
      int cy = 50 + 44 * i;
      g.fillEllipse(cx, cy, 26, 11, 0xFFFFFFu);
      g.fillEllipse(cx - 14, cy - 8, 14, 10, 0xFFFFFFu);
      g.fillEllipse(cx + 10, cy - 10, 16, 12, 0xFFFFFFu);
    }
  }
  g.fillEllipse(kScreenW / 2, kGroundY + 62, 210, 76, night ? 0x2C3A5Eu : 0x8FD97Au);
}

void App::drawStatusIcons(lgfx::LovyanGFX& g) {
  const uint32_t ink = pet_.mood() == Mood::Sleep ? 0xC9D1FFu : 0x33415Cu;
  // Battery, top right.
  const int bx = kScreenW - 36, by = 8;
  g.drawRoundRect(bx, by, 26, 13, 3, ink);
  g.fillRect(bx + 26, by + 4, 2, 5, ink);
  int level = battery_ < 0 ? 0 : (battery_ > 100 ? 100 : battery_);
  g.fillRoundRect(bx + 2, by + 2, 2 + 20 * level / 100, 9, 2,
                  level < kBatteryLowPercent ? 0xE5484Du : ink);
  // Headphones, top left.
  if (hal_.headphones()) {
    g.fillArc(20, 18, 8, 10, 180, 360, ink);
    g.fillRoundRect(8, 16, 5, 9, 2, ink);
    g.fillRoundRect(27, 16, 5, 9, 2, ink);
  }
}

void App::drawVolume(lgfx::LovyanGFX& g) {
  const int w = 176, h = 30, x = (kScreenW - w) / 2, y = 30;
  g.fillRoundRect(x, y, w, h, h / 2, 0x2B2D42u);
  // Speaker glyph.
  g.fillRect(x + 14, y + 11, 5, 8, 0xFFFFFFu);
  g.fillTriangle(x + 19, y + 11, x + 26, y + 6, x + 26, y + 24, 0xFFFFFFu);
  g.fillTriangle(x + 19, y + 11, x + 19, y + 19, x + 26, y + 24, 0xFFFFFFu);
  for (int i = 0; i < kVolumeSteps; ++i) {
    int bh = 5 + i * 15 / kVolumeSteps;
    g.fillRoundRect(x + 36 + i * 13, y + h - 6 - bh, 9, bh, 2,
                    i < volume_ ? 0x7CE38Bu : 0x565A75u);
  }
}

void App::drawStrip(lgfx::LovyanGFX& g) {
  g.fillRect(0, kStripY, kScreenW, kScreenH - kStripY, 0x2B2D42u);
  g.fillRect(0, kStripY, kScreenW, 3, pet_.mood() == Mood::Sleep ? 0x1B2040u : 0x6BBF59u);

  const char* title = hasMusic_ ? hal_.trackTitle(folder_, track_) : "Sem músicas";
  char sub[96];
  if (hasMusic_)
    snprintf(sub, sizeof sub, "%s  %d/%d", hal_.folderName(folder_), track_ + 1,
             hal_.trackCount(folder_));
  else
    snprintf(sub, sizeof sub, "Coloque MP3 no cartão");

  // Play / pause glyph.
  const int ix = 14, iy = kStripY + 26;
  if (playing_) {
    g.fillTriangle(ix, iy - 9, ix, iy + 9, ix + 15, iy, 0x7CE38Bu);
  } else {
    g.fillRoundRect(ix, iy - 9, 5, 18, 2, 0xFFD166u);
    g.fillRoundRect(ix + 9, iy - 9, 5, 18, 2, 0xFFD166u);
  }

  // Title, scrolling when it does not fit.
  const int tx = 40, tw = kScreenW - tx - 10;
  g.setFont(kTitleFont);
  g.setTextSize(1);
  g.setTextDatum(lgfx::middle_left);
  g.setTextColor(0xFFFFFFu);
  int width = g.textWidth(title);
  g.setClipRect(tx, kStripY + 8, tw, 36);
  if (width <= tw) {
    g.drawString(title, tx, iy);
  } else {
    const int gap = 50, span = width + gap;
    float t = titleScroll_ > 1.5f ? (titleScroll_ - 1.5f) * 30 : 0;   // pause, then scroll
    int off = static_cast<int>(t) % span;
    g.drawString(title, tx - off, iy);
    g.drawString(title, tx - off + span, iy);
  }
  g.clearClipRect();

  // Folder and track number on the left, elapsed time on the right.
  g.setFont(kSmallFont);
  g.setTextColor(0xA6ABC8u);
  g.setClipRect(14, kStripY + 44, 160, 22);
  g.drawString(sub, 14, kStripY + 55);
  g.clearClipRect();
  int pos = loaded_ ? hal_.positionSec() : 0;
  int dur = loaded_ ? hal_.durationSec() : 0;
  if (hasMusic_) {
    char tm[16];
    snprintf(tm, sizeof tm, "%d:%02d", pos / 60, pos % 60);
    g.setTextDatum(lgfx::middle_right);
    g.drawString(tm, kScreenW - 14, kStripY + 55);
  }

  // Progress bar.
  const int px = 14, py = kStripY + 72, pw = kScreenW - 28, ph = 6;
  g.fillRoundRect(px, py, pw, ph, 3, 0x565A75u);
  if (dur > 0) {
    int fill = pw * (pos > dur ? dur : pos) / dur;
    if (fill > 0) g.fillRoundRect(px, py, fill < ph ? ph : fill, ph, 3, 0x7CE38Bu);
  }
}

}  // namespace zpod
