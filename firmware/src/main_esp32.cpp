// zPod on the ESP32-S3: display, SD card, audio, buttons, battery and deep sleep.
#ifdef ARDUINO

#include <Arduino.h>
#include <Audio.h>
#include <FS.h>
#include <LovyanGFX.hpp>
#include <Preferences.h>
#include <SD_MMC.h>
#include <driver/rtc_io.h>
#include <esp_sleep.h>

#include <algorithm>
#include <string>
#include <vector>

#include "app.h"
#include "buttons.h"
#include "config.h"
#include "text.h"

using namespace zpod;

namespace {

// ------------------------------------------------------------------ display
class Display : public lgfx::LGFX_Device {
 public:
  Display() {
    {
      auto c = bus_.config();
      c.spi_host = SPI2_HOST;
      c.spi_mode = 0;
      c.freq_write = 40000000;
      c.freq_read = 16000000;
      c.use_lock = true;
      c.dma_channel = SPI_DMA_CH_AUTO;
      c.pin_sclk = kPinTftSck;
      c.pin_mosi = kPinTftMosi;
      c.pin_miso = -1;
      c.pin_dc = kPinTftDc;
      bus_.config(c);
      panel_.setBus(&bus_);
    }
    {
      auto c = panel_.config();
      c.pin_cs = kPinTftCs;
      c.pin_rst = kPinTftRst;
      c.pin_busy = -1;
      c.panel_width = kScreenW;
      c.panel_height = kScreenH;
      c.invert = true;        // IPS panels need inversion
      c.readable = false;
      c.bus_shared = false;
      panel_.config(c);
    }
    {
      auto c = light_.config();
      c.pin_bl = kPinTftBacklight;
      c.freq = 12000;
      c.pwm_channel = 7;
      light_.config(c);
      panel_.setLight(&light_);
    }
    setPanel(&panel_);
  }

 private:
  lgfx::Panel_ST7789 panel_;
  lgfx::Bus_SPI bus_;
  lgfx::Light_PWM light_;
};

Display lcd;
LGFX_Sprite canvas(&lcd);
Audio audio;
SemaphoreHandle_t audioLock;

struct AudioGuard {
  AudioGuard() { xSemaphoreTake(audioLock, portMAX_DELAY); }
  ~AudioGuard() { xSemaphoreGive(audioLock); }
};

// The decoder must be fed constantly; it gets its own task so drawing never starves it.
void audioTask(void*) {
  for (;;) {
    {
      AudioGuard g;
      audio.loop();
    }
    vTaskDelay(1);
  }
}

// ------------------------------------------------------------------ library
struct Folder {
  std::string name;
  std::string path;
  std::vector<std::string> files;    // file names inside `path`
  std::vector<std::string> titles;
};

bool isMp3(const std::string& n) {
  if (n.size() < 5 || n[0] == '.') return false;   // skips macOS "._" companions too
  std::string ext = n.substr(n.size() - 4);
  for (auto& c : ext) c = (char)tolower((unsigned char)c);
  return ext == ".mp3";
}

void scanFolder(Folder& f) {
  File dir = SD_MMC.open(f.path.c_str());
  if (!dir) return;
  for (File e = dir.openNextFile(); e; e = dir.openNextFile()) {
    std::string n = e.name();
    if (!e.isDirectory() && isMp3(n)) f.files.push_back(n);
  }
  std::sort(f.files.begin(), f.files.end(), [](const std::string& a, const std::string& b) {
    return strcasecmp(a.c_str(), b.c_str()) < 0;
  });
  for (auto& n : f.files) f.titles.push_back(titleFromFileName(n));
}

std::vector<Folder> scanLibrary() {
  std::vector<Folder> lib;
  File root = SD_MMC.open("/");
  if (!root) return lib;
  for (File e = root.openNextFile(); e; e = root.openNextFile()) {
    std::string n = e.name();
    if (!e.isDirectory() || n.empty() || n[0] == '.' || n == "System Volume Information") continue;
    Folder f;
    f.name = composeAccents(n);
    f.path = "/" + n;
    lib.push_back(std::move(f));
  }
  std::sort(lib.begin(), lib.end(), [](const Folder& a, const Folder& b) {
    return strcasecmp(a.name.c_str(), b.name.c_str()) < 0;
  });
  Folder loose;                      // MP3 files sitting in the card's root
  loose.name = "Músicas";
  loose.path = "/";
  lib.insert(lib.begin(), std::move(loose));
  for (auto& f : lib) scanFolder(f);
  lib.erase(std::remove_if(lib.begin(), lib.end(), [](const Folder& f) { return f.files.empty(); }),
            lib.end());
  return lib;
}

// ---------------------------------------------------------------------- hal
struct DeviceHal : Hal {
  std::vector<Folder> lib;
  Preferences prefs;
  Saved lastSaved;
  bool started = false, paused = false, phones = false;
  uint32_t startedMs = 0;
  int volumeStep = 0;
  float batteryMv = 0;

  int folderCount() override { return (int)lib.size(); }
  int trackCount(int f) override { return (int)lib[f].files.size(); }
  const char* folderName(int f) override { return lib[f].name.c_str(); }
  const char* trackTitle(int f, int t) override { return lib[f].titles[t].c_str(); }

  bool play(int f, int t) override {
    std::string path = lib[f].path;
    if (path.back() != '/') path += '/';
    path += lib[f].files[t];
    AudioGuard g;
    started = audio.connecttoFS(SD_MMC, path.c_str());
    paused = false;
    startedMs = millis();
    return started;
  }
  void setPaused(bool p) override {
    if (!started || p == paused) return;
    AudioGuard g;
    audio.pauseResume();
    paused = p;
  }
  void stop() override {
    AudioGuard g;
    audio.stopSong();
    started = false;
  }
  bool finished() override {
    if (!started || paused || millis() - startedMs < 1000) return false;
    AudioGuard g;
    return !audio.isRunning();
  }
  int positionSec() override {
    AudioGuard g;
    return (int)audio.getAudioCurrentTime();
  }
  int durationSec() override {
    AudioGuard g;
    return (int)audio.getAudioFileDuration();
  }
  void setVolume(int step) override {
    volumeStep = step;
    applyVolume();
  }
  void applyVolume() {
    int cap = phones ? kHeadphoneMaxVolume : kSpeakerMaxVolume;
    AudioGuard g;
    audio.setVolume((volumeStep * cap + kVolumeSteps / 2) / kVolumeSteps);
  }

  // Mutes the speaker and switches the volume cap when headphones come and go.
  void pollHeadphones() {
    bool now = (digitalRead(kPinHeadphoneDetect) == LOW) == kHeadphoneDetectActiveLow;
    if (now == phones) return;
    phones = now;
    digitalWrite(kPinAmpEnable, phones ? LOW : HIGH);
    applyVolume();
  }

  void setScreen(bool on) override {
    if (on) {
      lcd.wakeup();
      lcd.setBrightness(200);
    } else {
      lcd.setBrightness(0);
      lcd.sleep();
    }
  }
  int batteryPercent() override {
    float mv = analogReadMilliVolts(kPinBattery) * 2.0f;
    if (mv < 2500) return 100;       // no battery sense connected (bench / USB power)
    batteryMv = batteryMv == 0 ? mv : batteryMv * 0.8f + mv * 0.2f;
    int pct = (int)((batteryMv - 3400.0f) * 100.0f / (4150.0f - 3400.0f));
    return pct < 0 ? 0 : (pct > 100 ? 100 : pct);
  }
  bool headphones() override { return phones; }

  void powerOff() override {
    {
      AudioGuard g;
      audio.stopSong();
    }
    digitalWrite(kPinAmpEnable, LOW);
    gpio_hold_en((gpio_num_t)kPinAmpEnable);       // keep the amplifier shut down while asleep
    gpio_deep_sleep_hold_en();
    lcd.setBrightness(0);
    lcd.sleep();
    SD_MMC.end();
    while (digitalRead(kPinBtnPlay) == LOW) delay(20);   // wait for the long press to end
    delay(50);
    rtc_gpio_pullup_en((gpio_num_t)kPinBtnPlay);
    rtc_gpio_pulldown_dis((gpio_num_t)kPinBtnPlay);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)kPinBtnPlay, 0);                       // play button
    esp_sleep_enable_ext1_wakeup_io(1ULL << kPinTouch, ESP_EXT1_WAKEUP_ANY_HIGH);   // touch on top
    esp_deep_sleep_start();
  }

  void load(Saved& s) override {
    prefs.begin("zpod", false);
    s.folder = prefs.getInt("folder", 0);
    s.track = prefs.getInt("track", 0);
    s.volume = prefs.getInt("volume", -1);
    lastSaved = s;
  }
  void save(const Saved& s) override {
    if (s.folder != lastSaved.folder) prefs.putInt("folder", s.folder);
    if (s.track != lastSaved.track) prefs.putInt("track", s.track);
    if (s.volume != lastSaved.volume) prefs.putInt("volume", s.volume);
    lastSaved = s;
  }
};

DeviceHal hal;
App app(hal);
Buttons buttons;
uint32_t lastMs = 0;

}  // namespace

void setup() {
  Serial.begin(115200);
  gpio_hold_dis((gpio_num_t)kPinAmpEnable);
  pinMode(kPinAmpEnable, OUTPUT);
  digitalWrite(kPinAmpEnable, HIGH);
  pinMode(kPinHeadphoneDetect, INPUT_PULLUP);
  for (int pin : {kPinBtnPlay, kPinBtnPrev, kPinBtnNext, kPinBtnVolUp, kPinBtnVolDown})
    pinMode(pin, INPUT_PULLUP);
  pinMode(kPinTouch, INPUT);

  lcd.init();
  lcd.setRotation(0);
  lcd.setBrightness(0);
  canvas.setPsram(true);
  canvas.setColorDepth(16);
  canvas.createSprite(kScreenW, kScreenH);

  SD_MMC.setPins(kPinSdClk, kPinSdCmd, kPinSdD0);
  if (SD_MMC.begin("/sdcard", true)) hal.lib = scanLibrary();
  else Serial.println("SD card not found");
  Serial.printf("%d folders\n", (int)hal.lib.size());

  audioLock = xSemaphoreCreateMutex();
  audio.setPinout(kPinI2sBclk, kPinI2sLrck, kPinI2sDout);
  xTaskCreatePinnedToCore(audioTask, "audio", 8192, nullptr, 2, nullptr, 0);

  hal.pollHeadphones();
  app.begin();
  lastMs = millis();
}

void loop() {
  uint32_t now = millis();
  float dt = (now - lastMs) / 1000.0f;
  lastMs = now;

  auto emit = [](Button b, bool longPress) { app.onButton(b, longPress); };
  buttons.update(Button::Play, digitalRead(kPinBtnPlay) == LOW, now, emit);
  buttons.update(Button::Prev, digitalRead(kPinBtnPrev) == LOW, now, emit);
  buttons.update(Button::Next, digitalRead(kPinBtnNext) == LOW, now, emit);
  buttons.update(Button::VolUp, digitalRead(kPinBtnVolUp) == LOW, now, emit);
  buttons.update(Button::VolDown, digitalRead(kPinBtnVolDown) == LOW, now, emit);
  buttons.update(Button::Touch, digitalRead(kPinTouch) == HIGH, now, emit);

  hal.pollHeadphones();
  app.tick(dt);
  if (app.screenOn()) {
    app.render(canvas);
    canvas.pushSprite(0, 0);
  } else {
    delay(20);
  }
}

#endif  // ARDUINO
