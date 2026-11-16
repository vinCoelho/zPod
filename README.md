# zPod

A homemade MP3 player for a 6-year-old, with an on-screen virtual pet that reacts to the music.

- **Brain:** ESP32-S3 DevKitC-1 N16R8
- **Display:** 2.4" IPS ST7789 (240×320)
- **Audio:** MAX98357A (speaker) + PCM5102A (3.5 mm headphone jack)
- **Power:** LiPo 603450 1200 mAh, USB-C charging (TP4056)
- **Music:** internal microSD, Wi-Fi upload in "parent mode"
- **Case:** 3D-printed, 66 × 120 × 23 mm

## Structure

| Path | Contents |
|------|----------|
| [DECISIONS.md](DECISIONS.md) | Decision log, shopping list, pin map, risks |
| [case/zpod_case.py](case/zpod_case.py) | Parametric case model |
| [case/stl/](case/stl/) | STLs ready to slice |

![Case preview](case/preview.png)

## Regenerating the case

```bash
python3 -m venv case/.venv
case/.venv/bin/pip install -r case/requirements.txt
case/.venv/bin/python case/zpod_case.py
```

## Status

Design phase: decisions made, shopping list closed, case v0.1 generated from nominal dimensions. Firmware not started yet.
