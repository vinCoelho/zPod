// zPod configuration: pins, timings and limits. See DECISIONS.md for the rationale.
#pragma once
#include <stdint.h>

namespace zpod {

// ---- Screen -----------------------------------------------------------------
constexpr int kScreenW = 240;
constexpr int kScreenH = 320;

// ---- Pins (ESP32-S3 DevKitC-1 N16R8) ----------------------------------------
constexpr int kPinTftSck = 12, kPinTftMosi = 11, kPinTftCs = 10, kPinTftDc = 9, kPinTftRst = 14;
constexpr int kPinTftBacklight = 47;
constexpr int kPinSdClk = 39, kPinSdCmd = 41, kPinSdD0 = 40;
constexpr int kPinI2sBclk = 5, kPinI2sLrck = 6, kPinI2sDout = 7;
constexpr int kPinAmpEnable = 15;        // MAX98357A SD pin: high = speaker on
constexpr int kPinHeadphoneDetect = 16;  // PJ-307 switch
constexpr bool kHeadphoneDetectActiveLow = true;
constexpr int kPinBtnPlay = 2, kPinBtnPrev = 17, kPinBtnNext = 18, kPinBtnVolUp = 8, kPinBtnVolDown = 21;
constexpr int kPinTouch = 4;             // TTP223 output, active high
constexpr int kPinBattery = 1;           // 100k + 100k divider

// ---- Behaviour --------------------------------------------------------------
constexpr int kVolumeSteps = 10;         // what the child sees: 0..10
constexpr int kVolumeDefault = 5;
// Caps on the audio library's 0..21 scale; hearing protection, change only in parent mode.
constexpr int kSpeakerMaxVolume = 18;
constexpr int kHeadphoneMaxVolume = 12;

constexpr float kScreenTimeoutPlaying = 30.0f;   // s without input: screen off, music goes on
constexpr float kSleepTimeoutPaused = 120.0f;    // s without input while paused: pet falls asleep
constexpr float kSleepToPowerOff = 4.0f;         // s asleep before deep sleep
constexpr uint32_t kLongPressMs = 900;
constexpr uint32_t kRepeatMs = 250;
constexpr int kBatteryLowPercent = 15;
constexpr int kBatteryEmptyPercent = 4;

constexpr uint32_t kPetColor = 0x3B3350u;        // default body colour (RGB888)

}  // namespace zpod
