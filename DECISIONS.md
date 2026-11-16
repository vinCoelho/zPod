# zPod — Decision log

An MP3 player with a virtual pet for a 6-year-old. Built from scratch: electronics, firmware and a 3D-printed case.

## Premises (set by the project owner)

- User: a 6-year-old child → simple, sturdy, safe.
- Plays MP3, with easy navigation between songs.
- An on-screen pet that reacts to play, pause, skip and power on/off.
- A touch sensor on top of the device to wake the pet / leave standby.
- Case printed on a Bambu Lab A1 (256 × 256 × 256 mm build volume).
- Components bought on AliExpress.

## Decisions made

| # | Topic | Decision | Reason | Date |
|---|-------|----------|--------|------|
| 1 | Audio output | Built-in speaker + wired headphones (3.5 mm jack). No Bluetooth. | Works standalone, no pairing; wired headphones allow volume limiting. Frees us to use the ESP32-S3 (which has no Bluetooth audio). | 2026-11-16 |
| 2 | Display | ~2" rectangular color IPS (ST7789, 240×320) | Cheap, colorful, fits the pet plus the song name; smooth animation on the ESP32-S3. | 2026-11-16 |
| 3 | Controls | Large physical buttons: play/pause, previous, next, volume +, volume − | Usable without looking, sturdy, colorful printed caps. | 2026-11-16 |
| 4 | Assembly | Separate modules hand-wired and soldered (the builder is comfortable soldering) | Cheaper and more flexible; optimized case. | 2026-11-16 |
| 5 | Power on/off | Soft power: button + deep sleep, no physical switch cutting the battery | The pet must animate on power on and off, which needs the processor alive during the transition. (Follows from the premises; revisable.) | 2026-11-16 |
| 6 | Board | ESP32-S3 DevKitC-1 N16R8 (clone) | 16 MB flash + 8 MB PSRAM, plenty of pins, cheap and easy to replace. Battery charger goes on a separate module. | 2026-11-16 |
| 7 | Audio chain | MAX98357A (speaker) + PCM5102A (headphone jack) on the same I2S bus | Good quality, cheap; firmware mutes the speaker when headphones are detected; the PCM5102A's weak output helps limit volume. | 2026-11-16 |
| 8 | Battery | LiPo 603450 ~1200 mAh with protection | Thinner, lighter device; ~6–8 h of music. | 2026-11-16 |
| 9 | Loading music | Internal microSD (screwed in) + Wi-Fi upload in "parent mode" | Closed case, no loose parts; the child can't remove the card. | 2026-11-16 |
| 10 | Pet | Original little blob monster, drawn in code (vector shapes, not sprites) | Smooth animation, many expressions with little art work; the child picks its color and name. | 2026-11-16 |
| 11 | Form factor | Vertical, iPod-style: display on top, buttons in a cross below, touch on the top edge | Fits a small hand, simple internal layout. | 2026-11-16 |
| 12 | Printing | Single-color parts, no AMS; contrast comes from printing body and buttons in different colors | Works on any A1, no purge waste. | 2026-11-16 |
| 13 | Firmware | PlatformIO + Arduino (ESP32-audioI2S for MP3, LovyanGFX for display/pet, web server for upload) | Fastest path to a playing prototype. | 2026-11-16 |

### Derived technical decisions (Claude's recommendations — revisable)

| # | Topic | Decision | Reason |
|---|-------|----------|--------|
| T1 | Charging | TP4056 USB-C module with protection; its USB-C port is the device's only external port (charge only) | Simple and safe. The DevKit's USB stays inside the case, used only for the first flash. |
| T2 | Firmware updates | OTA over Wi-Fi, inside "parent mode" | No need to open the case to update. |
| T3 | 3.3 V regulation | Low-dropout LDO (RT9013 or HT7833) from the battery straight into the DevKit's 3V3 pin | The DevKit's regulator has ~1 V dropout and is unsuitable for LiPo. A good LDO gives ~89% efficiency with no switching converter (no audio noise). |
| T4 | Amplifier supply | MAX98357A powered directly from the battery (3.3–4.2 V) | ~1 W into the speaker: enough, and not too loud for a child. |
| T5 | Top touch | TTP223 module glued inside the top wall (1.2 mm thick at the pocket) | Simple digital output, wakes the ESP32 from deep sleep, draws ~3 µA. Module-free alternative: a native ESP32-S3 touch pin with copper tape. |
| T6 | SD card | Bus separate from the display (1-bit SD_MMC) | Avoids audio stutter while the display animates. |
| T7 | Headphones | PJ-307 jack with detect switch + 47–100 Ω series resistors per channel | Detects headphones to mute the speaker; resistors protect the PCM5102A and limit volume. |
| T8 | Volume limit | Software volume cap, separate for headphones and speaker, adjustable only in parent mode | Hearing protection. |
| T9 | Music | Each folder on the card is a "playlist"; holding next/previous switches folder | Simple navigation for someone still learning to read. |
| T10 | Fasteners | M2.5 × 6 screws into M2.5 brass heat-set inserts: 4 on the back cover and 4 on the button board | Opens and closes many times without wearing the plastic. |

## Risks and things to validate on the bench

1. **Standby current.** The DevKit has a power LED and a USB-serial chip that are always on; in deep sleep the whole thing may draw a few mA and drain 1200 mAh in 1–2 weeks. Measure with a multimeter. Mitigations, in order: (a) remove the DevKit's power LED; (b) build the soft-latch circuit (AO3401 + 2N7002), which truly cuts the battery on power off. The latch parts are already on the shopping list.
2. **Real module dimensions.** AliExpress sellers vary by a few millimetres. The case is parametric (`case/zpod_case.py`): measure everything with calipers on arrival and regenerate the STLs before the final print.
3. **PCM5102A driving headphones.** It is a line-level DAC, not a headphone amplifier. It works well with kids' headphones, but validate volume and distortion with the actual pair.
4. **LiPo battery from AliExpress.** Shipping to Brazil is sometimes blocked or slow; if so, buy the 603450 locally.

## Shopping list (AliExpress)

Prices estimated for November 2026 in Brazilian reais, taxes included, not checked listing by listing. Several items only sell in kits.

| # | Item | Search for | Qty | Estimate |
|---|------|-----------|-----|----------|
| 1 | Board | "ESP32-S3 DevKitC-1 N16R8", **headers not soldered** | 1 (ideally 2) | R$ 35–50 each |
| 2 | Display | "2.0 inch IPS TFT ST7789V 240x320 SPI", 8 pins | 1 | R$ 20–30 |
| 3 | Amplifier | "MAX98357A I2S amplifier module" | 1 | R$ 8–15 |
| 4 | DAC | "PCM5102A I2S DAC module" (GY-PCM5102) | 1 | R$ 15–25 |
| 5 | Speaker | 28 mm, 8 Ω, 2 W, up to 6 mm tall | 2 | R$ 8–15 |
| 6 | Battery | LiPo 603450, 3.7 V, 1200 mAh, with protection and leads | 1 | R$ 25–40 |
| 7 | Charger | "TP4056 Type-C with protection" (2 chips) | kit of 5 | R$ 8–15 |
| 8 | Regulator | "RT9013 3.3V LDO module" or "HT7833" | 2–5 | R$ 5–12 |
| 9 | SD reader | Small "Micro SD breakout", 3.3 V, no regulator | 2 | R$ 5–10 |
| 10 | Card | Brand-name microSD 16–32 GB (better bought locally) | 1 | R$ 25–35 |
| 11 | Touch | "TTP223 touch sensor module" | kit of 5–10 | R$ 5–10 |
| 12 | Buttons | Tactile switch 6×6×5 mm, 4 pins | kit of 20+ | R$ 6–10 |
| 13 | Headphone jack | "PJ-307" 3.5 mm stereo with switch | kit of 10 | R$ 6–10 |
| 14 | Perfboard | Universal 2.54 mm pitch, 5×7 cm (cut down for the buttons) | kit of 5 | R$ 8–12 |
| 15 | Wire | Silicone 28–30 AWG, assorted colors | kit | R$ 15–25 |
| 16 | Connectors | JST PH 2.0 2-pin, male + female with leads | kit of 10 | R$ 6–10 |
| 17 | Fasteners | Heat-set inserts M2.5 × 4 mm (3.5 mm OD), 8 pcs + M2.5 × 6 screws, 8 pcs | kit | R$ 20–30 |
| 18 | Soft latch (optional) | AO3401 (P-MOSFET), 2N7002, 1N5819 diode | kits | R$ 10–15 |
| 19 | Passives | 1/4 W resistor kit; 470 µF 10 V electrolytic capacitor | kit | R$ 12–18 |

**Estimated total: R$ 250–380**, excluding tools (soldering iron, multimeter, calipers) and filament.

## Pin map (ESP32-S3 DevKitC-1 N16R8)

Avoided: 0, 3, 45, 46 (boot), 19/20 (USB), 35–37 (PSRAM), 43/44 (serial), 48 (RGB LED).

| Function | GPIO | Note |
|----------|------|------|
| Display SCK / MOSI | 12 / 11 | SPI |
| Display CS / DC / RST | 10 / 9 / 14 | |
| Display backlight | 47 | PWM (brightness) |
| SD CLK / CMD / D0 | 39 / 41 / 40 | 1-bit SD_MMC |
| I2S BCLK / LRCK / DOUT | 5 / 6 / 7 | Goes to both audio modules |
| Amplifier enable | 15 | MAX98357A SD pin |
| Headphone detect | 16 | PJ-307 switch |
| Play/pause button (and power) | 2 | Wakes from deep sleep |
| Top touch (TTP223) | 4 | Wakes from deep sleep |
| Previous / next buttons | 17 / 18 | |
| Volume + / − buttons | 8 / 21 | |
| Battery voltage | 1 | 100 k + 100 k divider |

## Pet — planned states (to be detailed in the firmware phase)

| Event | Reaction |
|-------|----------|
| Power on | Wakes up, stretches, says hi |
| Play | Gets happy and dances to the beat |
| Pause | Stops, looks at the child, blinks slowly |
| Next / previous | Spins or hops toward that side |
| Volume | Eyes widen (up) / shrinks (down) |
| No interaction for a while | Yawns and sleeps (standby) |
| Touch on top | Wakes with a cute startle; if already awake, gets tickled |
| Low battery | Gets sleepy/hungry |
| Power off | Waves goodbye and lies down |

## Case (v0.1)

Parametric source in `case/zpod_case.py`; STLs in `case/stl/`; previews in `case/preview*.png`.

**Outer dimensions:** 66 × 114 × 23 mm, 10 mm corner radius, 2 mm walls.

| File | Part | How to print |
|------|------|--------------|
| `zpod_front.stl` | Tray: front face + 4 walls, display window, button holes, posts and bosses | Front face on the bed, no supports |
| `zpod_back.stl` | Cover with speaker grille and ring, TP4056 cradle | Outer face on the bed, no supports |
| `zpod_button_*.stl` (5) | Buttons with a raised symbol and a retaining flange | Flange on the bed, no supports; contrasting color |

Suggested: PLA or PETG, 0.2 mm layers (0.12 mm for the buttons), 3 walls, 15% infill.

**Internal layout in layers** (Z measured from the front face):

| Layer | Z (mm) | Contents |
|-------|--------|----------|
| A | 2–7 | Display (against the front face, located by 4 corner ribs) and buttons |
| B | 7–13.5 | Behind the display: battery (double-sided tape). Behind the buttons: perfboard with the 5 switches and the headphone jack, screwed onto 4 posts |
| C | 14–21 | DevKit in the centre, behind the battery; MAX98357A, PCM5102A, SD reader and LDO at the sides (double-sided tape). On the cover: speaker and TP4056 |

- **Button board:** posts on a 2.54 mm grid (38.1 × 35.56 mm between centres) to line up with perfboard holes (enlarge 4 holes to 2.7 mm). Buttons sit 15.24 mm (6 holes) from play.
- **Headphones:** PJ-307 soldered to the same board, exiting through a 7 mm hole in the bottom wall. Tilt the board to fit it in.
- **USB-C:** slot in the bottom wall; the TP4056 sits between rails on the cover, fixed with hot glue.
- **Touch:** 16 × 12 mm pocket in the top wall (leaving 1.2 mm) to glue the TTP223 into.
- **Display:** solder wires straight to the module (no pin header) and fix it with dabs of hot glue at the corner ribs.

**Case to-do**
- [ ] Measure the real display, TP4056, jack and speaker and adjust the PARAMETERS section.
- [ ] Print only the front first to test the display window and button travel.
- [ ] Consider a lanyard hole and a protective lens over the display.

## Next steps

1. Buy the components.
2. Breadboard everything and validate audio, display, SD and standby current.
3. Firmware: basic player → pet → parent mode (Wi-Fi upload + OTA).
4. Adjust the case to the real measurements and print.
