# Minimalist OLED 0.96" Modern Modular Cases — Assembly Guide

> Source: [MakerWorld model 2085334](https://makerworld.com/en/models/2085334-minimalist-oled-0-96-modern-modular-cases#profileId-2591428), profile **2591428 — 0.96" Blue “old” display**.
>
> This Markdown version is a local reference assembled from the model’s print-page description and profile notes.

## Important fit note for the old screen

To make the old 0.96" blue OLED screen fit and work correctly:

1. Use the **`0.96" old`** screen/front-panel part.
2. Use the **full build plate**.
3. Scale the full build plate to **103% of its original size** before printing.

Both conditions are required for the old screen fit.

## Design variants

The model is a modular family of clean, 30°-angled cases for 0.96" OLED displays:

- **Slim Case** — compact shell for the display only.
- **Stand** — angled stand for displaying data or icons.
- **Stand + ESP32-C3** — angled stand with space and mounting for an ESP32-C3 Super Mini. The controller can optionally be secured with two screws.
- **Full Enclosure** — enclosed housing for the OLED and ESP32-C3, secured with four screws.

The parts use snap-fit assembly and have a textured finish.

## Parts and tools

Choose the parts that match the intended variant:

- Printed case parts from the selected MakerWorld profile.
- A compatible 0.96" OLED display.
- An ESP32-C3 Super Mini when using either ESP32-C3 variant.
- Optional screws for the ESP32-C3 and full-enclosure variants.
- A USB cable and the electronics required by the project.

No glue is intended for the snap-fit joints. Have a small flat tool available only if a joint needs gentle separation during a fit check.

## Assembly procedure

### 1. Select the correct display front

Identify the display version before printing or assembling. For the older blue display, use the **`0.96" old`** part and print the **full build plate at 103% scale** as described above. Do not mix the old-display front with an unscaled full build plate.

### 2. Print the selected case parts

Print the plate(s) for the case variant you want. The original profile is configured around 0.2 mm layers, two walls, and 15% infill; adjust only if your own printer/material needs different settings.

After printing, remove supports or brim material if present and clean the snap-fit features. Check that the clips and mating edges are free of strings and blobs.

### 3. Dry-fit the case

Before installing electronics, bring the printed parts together without force:

1. Align the tabs, clips, and locating edges.
2. Press the parts together evenly until the snap fits engage.
3. Confirm that the display opening is centered and that the case sits at its intended 30° angle.
4. If a joint will not close, stop and clear the mating features rather than forcing it.

### 4. Install the OLED display

1. Place the OLED module into the display recess/front panel, with the screen facing outward.
2. Route the display wires through the intended opening without pinching them.
3. Verify that the display is seated flat and that the front panel does not press on the glass or PCB components.
4. Snap the display/front section into the matching case or stand section.

For the old screen, verify fit with the **103%-scaled full build plate** before connecting power.

### 5. Install the ESP32-C3 (if used)

1. Place the ESP32-C3 Super Mini in its pocket or mounting location.
2. Route the display and USB wires so they remain clear of the snap-fit edges.
3. For the Stand + ESP32-C3 option, install the optional two screws if extra retention is desired.
4. For the Full Enclosure option, close the enclosure and install all four specified screws.

Do not trap or sharply bend the wires when closing the case.

### 6. Final checks

- Confirm that every snap-fit is fully engaged.
- Confirm that the display is visible and not under mechanical stress.
- Confirm that USB access and cable routing remain usable.
- Power the electronics and verify the OLED before placing the case in service.

## Troubleshooting

### The old screen does not fit

Confirm that you used the **`0.96" old`** part and that the **full build plate was scaled to 103%**. A standard 100% plate is not the specified fit for this screen revision.

### A snap-fit will not close

Inspect both mating surfaces for stringing, elephant foot, or excess material. Clean the feature and retry with even pressure. Do not force a misaligned joint.

### The display is loose

Recheck that the selected front panel matches the screen revision and that the module is fully seated in its recess before closing the case.

## Source notes

- Model: **Minimalist OLED 0.96" Modern Modular Cases**
- MakerWorld design ID: **2085334**
- Selected profile: **2591428 — 0.96" Blue “old” display**
- MakerWorld page: <https://makerworld.com/en/models/2085334-minimalist-oled-0-96-modern-modular-cases#profileId-2591428>
- The print page describes snap-fit, 30°-angled modular cases and identifies the old blue-display update. It does not expose a separate text assembly-guide section; this file preserves those instructions as a local Markdown guide.
