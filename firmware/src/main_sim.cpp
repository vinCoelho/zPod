// Desktop simulator: the same pet and UI code in an SDL window, with a fake player.
//
//   Space = play/pause (hold = power off)     Left / Right = previous / next (hold = folder)
//   Up / Down = volume                        T = touch the top
//   H = plug/unplug headphones                B = drain the battery by 10%
//
//   --snapshot <dir> renders a few scripted frames to PPM files instead of opening a window.
#ifndef ARDUINO

#include <LovyanGFX.hpp>
#include <LGFX_AUTODETECT.hpp>
#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>

#include "app.h"
#include "buttons.h"
#include "config.h"

using namespace zpod;

namespace {

struct SimHal : Hal {
  struct Folder {
    const char* name;
    std::vector<const char*> tracks;
  };
  std::vector<Folder> lib = {
      {"Favoritas", {"Borboletinha", "A Dona Aranha", "Pintinho Amarelinho",
                     "O Sapo Não Lava o Pé e Outras Canções do Brejo"}},
      {"Filmes", {"Livre Estou", "Hakuna Matata", "Você Quer Brincar na Neve?"}},
      {"Hora de Dormir", {"Brilha Brilha Estrelinha", "Canção de Ninar"}},
  };
  float pos = 0;
  bool loaded = false, paused = false, phones = false, screen = true, off = false;
  int battery = 82, volume = 0;
  Saved saved;

  void advance(float dt) {
    if (loaded && !paused && pos < durationSec()) pos += dt;
  }

  int folderCount() override { return static_cast<int>(lib.size()); }
  int trackCount(int f) override { return static_cast<int>(lib[f].tracks.size()); }
  const char* folderName(int f) override { return lib[f].name; }
  const char* trackTitle(int f, int t) override { return lib[f].tracks[t]; }
  bool play(int, int) override { pos = 0; loaded = true; paused = false; return true; }
  void setPaused(bool p) override { paused = p; }
  void stop() override { loaded = false; pos = 0; }
  bool finished() override { return loaded && pos >= durationSec(); }
  int positionSec() override { return static_cast<int>(pos); }
  int durationSec() override { return 25; }   // short fake songs, to see track changes
  void setVolume(int v) override { volume = v; }
  void setScreen(bool on) override { screen = on; }
  int batteryPercent() override { return battery; }
  bool headphones() override { return phones; }
  void powerOff() override { off = true; }
  void load(Saved& s) override { s = saved; }
  void save(const Saved& s) override { saved = s; }
};

SimHal hal;
App app(hal);

void writePpm(LGFX_Sprite& c, const std::string& path) {
  FILE* f = fopen(path.c_str(), "wb");
  if (!f) { perror(path.c_str()); return; }
  fprintf(f, "P6\n%d %d\n255\n", kScreenW, kScreenH);
  for (int y = 0; y < kScreenH; ++y)
    for (int x = 0; x < kScreenW; ++x) {
      auto px = c.readPixelRGB(x, y);
      fputc(px.r, f); fputc(px.g, f); fputc(px.b, f);
    }
  fclose(f);
}

// Runs a scripted session without a window and saves one frame per scene.
int snapshot(const char* dir) {
  LGFX_Sprite canvas;
  canvas.setColorDepth(16);
  canvas.createSprite(kScreenW, kScreenH);
  int n = 0;
  auto run = [&](float seconds) {
    for (float t = 0; t < seconds; t += 1 / 30.0f) { hal.advance(1 / 30.0f); app.tick(1 / 30.0f); }
  };
  auto shot = [&](const char* name) {
    app.render(canvas);
    char path[512];
    snprintf(path, sizeof path, "%s/%02d_%s.ppm", dir, n++, name);
    writePpm(canvas, path);
    printf("%s  screen=%d\n", path, app.screenOn());
  };
  app.begin();
  run(0.5f); shot("hello");
  run(3.0f); shot("idle");
  app.onButton(Button::Play, false); run(0.38f); shot("dance");
  app.onButton(Button::Next, false); run(0.25f); shot("next");
  app.onButton(Button::VolUp, false); run(0.3f); shot("volume_up");
  run(2.0f);
  app.onButton(Button::Next, false); run(1.0f);
  app.onButton(Button::Next, false); run(4.0f); shot("long_title");
  app.onButton(Button::Touch, false); run(0.4f); shot("tickle");
  hal.phones = true; hal.battery = 9; run(6.0f); shot("low_battery");
  hal.battery = 60; run(6.0f);
  app.onButton(Button::Play, false); run(kSleepTimeoutPaused + 1.0f); shot("yawn");
  run(3.0f); shot("sleep");
  app.onButton(Button::Touch, false); run(0.25f); shot("startle");
  run(2.0f);
  app.onButton(Button::Play, true); run(0.6f); shot("bye");
  run(4.0f);
  printf("powered off=%d\n", hal.off);
  return 0;
}

#if defined(SDL_h_)
LGFX lcd(kScreenW, kScreenH, 2);
LGFX_Sprite canvas(&lcd);
Buttons buttons;

int simLoop(bool* running) {
  lcd.init();
  canvas.setColorDepth(16);
  canvas.createSprite(kScreenW, kScreenH);
  app.begin();

  struct Key { SDL_Scancode code; Button button; };
  const Key keys[] = {{SDL_SCANCODE_SPACE, Button::Play}, {SDL_SCANCODE_LEFT, Button::Prev},
                      {SDL_SCANCODE_RIGHT, Button::Next}, {SDL_SCANCODE_UP, Button::VolUp},
                      {SDL_SCANCODE_DOWN, Button::VolDown}, {SDL_SCANCODE_T, Button::Touch}};
  uint32_t last = lgfx::millis();
  bool hDown = false, bDown = false;
  while (*running) {
    uint32_t now = lgfx::millis();
    float dt = (now - last) / 1000.0f;
    last = now;

    const uint8_t* kb = SDL_GetKeyboardState(nullptr);
    for (auto& k : keys)
      buttons.update(k.button, kb[k.code], now, [&](Button b, bool lng) {
        if (hal.off) hal.off = false;
        app.onButton(b, lng);
      });
    if (kb[SDL_SCANCODE_H] && !hDown) hal.phones = !hal.phones;
    if (kb[SDL_SCANCODE_B] && !bDown) hal.battery = hal.battery >= 10 ? hal.battery - 10 : 100;
    hDown = kb[SDL_SCANCODE_H];
    bDown = kb[SDL_SCANCODE_B];

    hal.advance(dt);
    app.tick(dt);
    if (app.screenOn()) app.render(canvas);
    else canvas.fillScreen(0x000000u);
    canvas.pushSprite(0, 0);
    lgfx::delay(16);
  }
  return 0;
}
#endif

}  // namespace

int main(int argc, char** argv) {
  if (argc >= 3 && strcmp(argv[1], "--snapshot") == 0) return snapshot(argv[2]);
#if defined(SDL_h_)
  return lgfx::Panel_sdl::main(simLoop);
#else
  fprintf(stderr, "Built without SDL2; only --snapshot <dir> is available.\n");
  return 1;
#endif
}

#endif  // !ARDUINO
