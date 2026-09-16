# DeskBuddy wiring guide

DeskBuddy's primary target is an ESP32-C3 Super Mini and a 128×64 I²C SSD1306 OLED.

## ESP32-C3 primary profile

| OLED pin | ESP32-C3 pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO8 |
| SCL | GPIO9 |

The default OLED address is `0x3C`. Power the display from 3.3 V and keep the I²C wires short. Confirm the labels on the particular board before applying power.

## Classic ESP32 fallback profile

When compiled for a classic ESP32 target, `config.h` selects GPIO21/GPIO22:

| OLED pin | Classic ESP32 pin |
|---|---|
| VCC | 3V3 |
| GND | GND |
| SDA | GPIO21 |
| SCL | GPIO22 |

The C3 profile is the primary tested target. The classic profile exists as a compatibility fallback and does not require the removed MPU or touch hardware.

## Software profile

`deskbuddy/config.h` defines the active board profile:

```text
C3 SDA: GPIO8
C3 SCL: GPIO9
Classic SDA: GPIO21
Classic SCL: GPIO22
OLED: 0x3C
I²C: 400 kHz
```

DeskBuddy uses only the OLED and ESP32. No MPU6050, MPU6500, or touch sensor is required.

## Arduino setup

1. Disconnect USB power while wiring.
2. Connect 3V3 and GND first.
3. Connect SDA and SCL according to the selected profile.
4. Install Adafruit GFX, Adafruit SSD1306, and FluxGarage RoboEyes.
5. Select the board and correct serial port.
6. Compile before flashing.

If the OLED is not detected, check the module address, power, ground, SDA/SCL labels, and selected board profile. Generic OLED modules may use different controllers or addresses even when their connectors look identical.
