#include "pet.h"

#include <math.h>

namespace zpod {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kBodyRx = 60, kBodyRy = 54;

float duration(Reaction r) {
  switch (r) {
    case Reaction::Hello: return 1.6f;
    case Reaction::Bye: return 1.8f;
    case Reaction::HopLeft:
    case Reaction::HopRight: return 0.5f;
    case Reaction::BigEyes:
    case Reaction::Shrink: return 0.6f;
    case Reaction::Startle: return 0.8f;
    case Reaction::Tickle: return 1.0f;
    case Reaction::Yawn: return 2.0f;
    default: return 0;
  }
}

uint32_t mix(uint32_t a, uint32_t b, float k) {
  auto ch = [&](int shift) {
    float ca = (a >> shift) & 0xFF, cb = (b >> shift) & 0xFF;
    return static_cast<uint32_t>(ca + (cb - ca) * k) & 0xFF;
  };
  return (ch(16) << 16) | (ch(8) << 8) | ch(0);
}

}  // namespace

void Pet::react(Reaction r) {
  reaction_ = r;
  reactionTime_ = 0;
}

void Pet::update(float dt) {
  t_ += dt;
  blink_ -= dt;
  if (blink_ < 0) blink_ = 2.5f + 2.5f * (0.5f + 0.5f * sinf(t_ * 12.9898f));
  if (reaction_ != Reaction::None) {
    reactionTime_ += dt;
    if (reactionTime_ >= duration(reaction_)) reaction_ = Reaction::None;
  }
}

Pet::Pose Pet::pose() const {
  Pose p;
  switch (mood_) {
    case Mood::Idle: {
      float b = sinf(t_ * 2.0f);
      p.sy = 1 + 0.025f * b;
      p.sx = 1 - 0.02f * b;
      p.px = 5 * sinf(t_ * 0.7f);
      p.py = 2 * sinf(t_ * 0.9f);
      if (blink_ < 0.12f) p.eyeOpen = 0.1f;
      break;
    }
    case Mood::Dance: {
      float beat = t_ * 2.1f;
      float bounce = fabsf(sinf(kPi * beat));
      p.lift = 16 * bounce;
      p.sy = 0.9f + 0.14f * bounce;
      p.sx = 1.08f - 0.1f * bounce;
      p.dx = 12 * sinf(kPi * beat * 0.5f);
      p.happyEyes = fmodf(beat, 8) < 2;
      p.mouth = Mouth::Open;
      if (blink_ < 0.12f) p.eyeOpen = 0.1f;
      break;
    }
    case Mood::Sleep:
      p.sy = 0.82f + 0.03f * sinf(t_ * 1.2f);
      p.sx = 1.12f;
      p.eyeOpen = 0;
      p.mouth = Mouth::Small;
      break;
  }
  if (tired_ && mood_ != Mood::Sleep) {
    if (p.eyeOpen > 0.5f) p.eyeOpen = 0.5f;
    p.happyEyes = false;
    p.mouth = Mouth::Flat;
  }

  if (reaction_ == Reaction::None) return p;
  float k = reactionTime_ / duration(reaction_);   // 0..1
  float bump = sinf(kPi * k);                      // 0 -> 1 -> 0
  switch (reaction_) {
    case Reaction::Hello:
      p.eyeOpen = 1;
      p.happyEyes = false;
      if (k < 0.5f) p.lift += 22 * fabsf(sinf(kPi * k * 4));
      p.wave = k * 5;
      p.mouth = Mouth::Open;
      break;
    case Reaction::Bye:
      p.wave = k * 5;
      p.happyEyes = false;
      p.mouth = Mouth::Smile;
      p.eyeOpen = k < 0.6f ? 1 : 1 - (k - 0.6f) / 0.4f;
      if (k > 0.6f) p.sy *= 1 - 0.15f * (k - 0.6f) / 0.4f;
      break;
    case Reaction::HopLeft:
    case Reaction::HopRight: {
      float dir = reaction_ == Reaction::HopLeft ? -1 : 1;
      p.dx += dir * 36 * bump;
      p.lift += 26 * bump;
      p.px = dir * 6;
      p.eyeOpen = 1;
      p.happyEyes = false;
      break;
    }
    case Reaction::BigEyes:
      p.eyeOpen = 1;
      p.happyEyes = false;
      p.eyeScale = 1 + 0.35f * bump;
      p.sy += 0.08f * bump;
      p.lift += 6 * bump;
      p.mouth = Mouth::Small;
      break;
    case Reaction::Shrink:
      p.sx *= 1 - 0.14f * bump;
      p.sy *= 1 - 0.16f * bump;
      p.eyeScale = 1 - 0.15f * bump;
      p.mouth = Mouth::Small;
      break;
    case Reaction::Startle: {
      float j = k < 0.3f ? k / 0.3f : 1 - (k - 0.3f) / 0.7f;
      p.lift += 30 * j;
      p.sx = 1;
      p.sy = 1 + 0.08f * j;
      p.eyeOpen = 1;
      p.happyEyes = false;
      p.eyeScale = 1 + 0.4f * j;
      p.mouth = Mouth::Small;
      break;
    }
    case Reaction::Tickle:
      p.dx += 6 * sinf(k * 40);
      p.sx *= 1 + 0.04f * sinf(k * 40);
      p.happyEyes = true;
      p.mouth = Mouth::Open;
      break;
    case Reaction::Yawn:
      p.mouth = Mouth::Yawn;
      p.mouthSize = bump;
      p.eyeOpen = 1 - 0.9f * bump;
      p.happyEyes = false;
      p.sy += 0.05f * bump;
      break;
    default: break;
  }
  return p;
}

void Pet::draw(lgfx::LovyanGFX& g, int cx, int groundY, uint32_t shadowColor) const {
  const Pose p = pose();
  const uint32_t body = color_;
  const uint32_t belly = 0xD9C2FAu;                  // lilac tummy and muzzle
  const uint32_t dark = mix(body, 0x000000u, 0.45f);
  const uint32_t ink = 0x2B2140u;
  const uint32_t pink = 0xFF5FA2u;
  const uint32_t line = 0xFFFFFFu;                   // closed eyes, drawn on the dark body
  const uint32_t bolt = 0xFFD23Fu;

  const float rx = kBodyRx * p.sx, ry = kBodyRy * p.sy;
  const float x = cx + p.dx;
  const float y = groundY - ry - p.lift;

  // Shadow shrinks as the pet leaves the ground.
  float sh = 1 - fminf(p.lift, 40) / 80;
  g.fillEllipse(x, groundY + 3, 54 * p.sx * sh, 7 * sh, shadowColor);

  // Little tail with a heart tip, wagging behind the body.
  {
    float wag = 5 * sinf(t_ * 4);
    float bx = x - rx + 6, by = y + ry * 0.45f;
    float mx = bx - 16, my2 = by - 4 + wag * 0.4f;
    float tx = bx - 26, ty = by - 22 + wag;
    g.drawWideLine(bx, by, mx, my2, 2.0f, dark);
    g.drawWideLine(mx, my2, tx, ty, 2.0f, dark);
    g.fillSmoothCircle(tx - 3, ty - 2, 4, pink);
    g.fillSmoothCircle(tx + 3, ty - 2, 4, pink);
    g.fillTriangle(tx - 7, ty - 1, tx + 7, ty - 1, tx, ty + 8, pink);
  }

  // Pointed ears with pink tips, leaning outwards and wobbling a little.
  for (int s = -1; s <= 1; s += 2) {
    float wob = 3 * sinf(t_ * 3 + s) + p.dx * 0.15f;
    float b0x = x + s * 12 * p.sx, b1x = x + s * 44 * p.sx, by = y - ry * 0.62f;
    float tx = x + s * 50 * p.sx - wob, ty = y - ry - 30;
    g.fillTriangle(b0x, by - 12, b1x, by + 10, tx, ty, body);
    float kx = tx + (b0x + b1x - 2 * tx) * 0.19f, ky = ty + (by - ty) * 0.38f;
    g.fillTriangle(tx, ty, kx - s * 7, ky - 2, kx + s * 6, ky + 5, pink);
  }

  // Pink mohawk between the ears.
  for (int i = -2; i <= 2; ++i) {
    float mx = x + i * 7 * p.sx, top = y - ry - 20 + abs(i) * 5 + 2 * sinf(t_ * 5 + i);
    g.fillTriangle(mx - 6, y - ry + 8, mx + 6, y - ry + 8, mx + i * 2, top, pink);
  }

  // Feet.
  for (int s = -1; s <= 1; s += 2)
    g.fillEllipse(x + s * 26 * p.sx, groundY - p.lift - 3, 17, 8, dark);

  // Arms: resting at the sides, or one raised and waving.
  for (int s = -1; s <= 1; s += 2) {
    if (s == 1 && p.wave >= 0) {
      float a = 0.5f * sinf(p.wave * 2 * kPi);
      g.drawWideLine(x + rx - 8, y - 4, x + rx + 14 + 10 * a, y - 34, 5, body);
      g.fillSmoothCircle(x + rx + 14 + 10 * a, y - 36, 10, body);
    } else {
      g.fillSmoothCircle(x + s * (rx - 2), y + 14, 11, body);
    }
  }

  // Body and belly.
  g.fillEllipse(x, y, rx, ry, body);
  g.fillEllipse(x, y + ry * 0.38f, rx * 0.62f, ry * 0.45f, belly);

  // Cheeks.
  for (int s = -1; s <= 1; s += 2) g.fillEllipse(x + s * 41 * p.sx, y + 6 * p.sy, 8, 5, pink);

  // Eyes.
  const float ey = y - 10 * p.sy;
  for (int s = -1; s <= 1; s += 2) {
    float ex = x + s * 23 * p.sx;
    if (p.happyEyes) {
      g.fillArc(ex, ey + 6, 9, 13, 200, 340, line);         // ^ ^
    } else if (p.eyeOpen < 0.2f) {
      g.fillArc(ex, ey - 6, 9, 12, 20, 160, line);          // closed
    } else {
      float erx = 14 * p.eyeScale, ery = 16 * p.eyeScale * p.eyeOpen;
      g.fillEllipse(ex, ey, erx, ery, 0xFFFFFFu);
      float pr = 7.5f * p.eyeScale;
      g.fillEllipse(ex + p.px, ey + p.py, pr, fminf(pr, ery - 1), ink);
      if (ery > 8) g.fillSmoothCircle(ex + p.px - 3, ey + p.py - 3, 2, 0xFFFFFFu);
      // Confident eyelid, lower towards the nose; hidden when the eyes go wide.
      if (p.eyeScale < 1.1f && ery > 8)
        g.fillTriangle(ex - s * (erx + 2), ey - ery - 3, ex + s * (erx + 2), ey - ery - 3,
                       ex - s * (erx + 2), ey - ery * 0.45f, body);
    }
  }

  // Lightning bolt on the forehead.
  {
    float hx = x, hy = y - ry * 0.66f;
    g.fillTriangle(hx + 3, hy - 9, hx - 7, hy + 3, hx + 1, hy + 3, bolt);
    g.fillTriangle(hx - 3, hy + 13, hx + 7, hy + 1, hx - 1, hy + 1, bolt);
  }

  // Mouth.
  const float my = y + 16 * p.sy;
  switch (p.mouth) {
    case Mouth::Smile: g.fillArc(x, my - 5, 9, 12, 25, 155, ink); break;
    case Mouth::Open:
      g.fillEllipse(x, my + 2, 11, 9, ink);
      g.fillEllipse(x, my + 6, 6, 4, 0xFF5C7Au);
      break;
    case Mouth::Small: g.fillSmoothCircle(x, my + 2, 4, ink); break;
    case Mouth::Flat: g.drawWideLine(x - 7, my + 2, x + 7, my + 2, 1.5f, ink); break;
    case Mouth::Yawn:
      g.fillEllipse(x, my + 3, 6 + 6 * p.mouthSize, 3 + 13 * p.mouthSize, ink);
      if (p.mouthSize > 0.4f) g.fillEllipse(x, my + 8 + 6 * p.mouthSize, 5, 3, 0xFF5C7Au);
      break;
  }

  // Electric guitar while rocking out.
  if (mood_ == Mood::Dance && !tired_ && reaction_ == Reaction::None) {
    float gx = x - 10, gy = y + ry * 0.62f;
    g.drawWideLine(gx + 6, gy - 4, x + rx + 20, y - 14, 3.0f, 0xF2E9FFu);      // neck
    g.fillSmoothCircle(x + rx + 22, y - 16, 5, bolt);                          // headstock
    g.fillEllipse(gx - 8, gy + 4, 19, 13, pink);                               // guitar body
    g.fillEllipse(gx + 8, gy - 3, 11, 9, pink);
    g.fillEllipse(gx - 8, gy + 4, 7, 5, 0xFFFFFFu);
    g.fillSmoothCircle(gx - 26, gy + 4, 3, bolt);
    g.fillSmoothCircle(gx - 2 + 4 * sinf(t_ * 13), gy - 9, 8, body);           // strumming hand
    g.fillSmoothCircle(x + rx + 4, y - 4, 9, body);                            // hand on the neck
  }

  // Mood extras.
  if (mood_ == Mood::Dance && !tired_) {
    for (int i = 0; i < 3; ++i) {                           // music notes drifting up
      float ph = fmodf(t_ * 0.45f + i * 0.37f, 1.0f);
      float nx = cx + (i == 1 ? 92 : (i == 0 ? -92 : -70)) + 6 * sinf(t_ * 2 + i);
      float ny = groundY - 60 - ph * 110;
      uint32_t c = (i == 0) ? 0xFF5FA2u : (i == 1 ? 0xFFFFFFu : 0xFFD23Fu);
      g.fillEllipse(nx, ny, 6, 5, c);
      g.drawWideLine(nx + 5, ny, nx + 5, ny - 18, 1.5f, c);
      g.drawWideLine(nx + 5, ny - 18, nx + 12, ny - 14, 1.5f, c);
    }
  } else if (mood_ == Mood::Sleep && reaction_ == Reaction::None) {
    g.setFont(&fonts::FreeSansBold12pt7b);
    g.setTextDatum(lgfx::middle_center);
    for (int i = 0; i < 3; ++i) {                           // z z z
      float ph = fmodf(t_ * 0.3f + i * 0.33f, 1.0f);
      g.setTextSize(0.6f + 0.7f * ph);
      g.setTextColor(mix(0x9AA5FFu, 0x232946u, ph * 0.7f));
      g.drawString("z", cx + 50 + 34 * ph, groundY - 110 - 70 * ph);
    }
    g.setTextSize(1);
  }
}

}  // namespace zpod
