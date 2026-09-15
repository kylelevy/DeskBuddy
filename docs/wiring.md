# YETI hardware wiring guide

This guide matches the pin assignments used by `yeti_v1/yeti_v1.ino`. YETI uses one I²C bus for the SSD1306 OLED and the MPU6050/MPU6500 motion sensor, so the two devices share SDA, SCL, power, and ground.

## Parts

- ESP32-C3 Super Mini (primary small-build profile), **or** a classic ESP32 DevKit/WROOM
- 0.96-inch SSD1306 OLED, 128×64, with four pins: `VCC`, `GND`, `SCL`, `SDA`
- MPU6050/MPU6500-style I²C motion-sensor module
- Jumper wires and a USB cable or regulated 3.3 V supply

## Important power note

Power the OLED and motion sensor from the controller's **3.3 V (`3V3`) rail**, not 5 V. Connect every module's ground to the ESP32 ground. Do not connect a module's `VCC` to `VIN` or `5V` unless that exact module is documented as 5 V-safe and its I/O is level-shifted.

## ESP32-C3 Super Mini (primary profile)

The firmware's active `YETI_BOARD_ESP32_C3_MINI` definitions are:

- SDA: **GPIO8**
- SCL: **GPIO9**
- OLED I²C address: `0x3C`
- MPU I²C address: `0x68`

Wire the OLED as follows:

| OLED pin | Connect to ESP32-C3 Super Mini |
|---|---|
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SDA` | `GPIO8` |
| `SCL` | `GPIO9` |

Wire the motion sensor to the same bus:

| MPU6050/MPU6500 pin | Connect to ESP32-C3 Super Mini |
|---|---|
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SDA` | `GPIO8` |
| `SCL` | `GPIO9` |

The OLED and MPU are connected **in parallel** on SDA and SCL; do not use separate GPIO pairs for them.

> **Pin-label caution:** C3 Super Mini board silkscreens vary between manufacturers. Confirm the board's `GPIO8`, `GPIO9`, `3V3`, and `GND` labels or pinout before applying power. The firmware source contains an older comment mentioning GPIO1/GPIO3; the executable configuration and `Wire.begin()` call use GPIO8/GPIO9, which are the connections documented here.

## ASCII wiring diagram

The primary ESP32-C3 build is wired as one shared I²C bus. The `SDA` and `SCL` lines branch to both modules; power and ground are shared as well.

```text
                         ESP32-C3 Super Mini
                    +-------------------------+
                    |                         |
       3V3 --------+----+--------------------+---- OLED VCC
                    |    |                    |
                    |    +------------------------ MPU VCC
                    |                         |
       GND --------+----+--------------------+---- OLED GND
                    |    |                    |
                    |    +------------------------ MPU GND
                    |                         |
     GPIO8 (SDA) --+----+--------------------+---- OLED SDA
                    |    |                    |
                    |    +------------------------ MPU SDA
                    |                         |
     GPIO9 (SCL) --+----+--------------------+---- OLED SCL
                    |    |                    |
                    |    +------------------------ MPU SCL
                    +-------------------------+

       OLED and MPU are parallel connections on the same I²C bus.
```

For a classic ESP32 DevKit/WROOM, use the same topology but replace `GPIO8` with `GPIO21` and `GPIO9` with `GPIO22`.

## Classic ESP32 DevKit / WROOM profile

When the firmware is compiled for a classic ESP32 target, it selects:

- SDA: **GPIO21**
- SCL: **GPIO22**
- OLED I²C address: `0x3C`
- MPU I²C address: `0x68`

Wire both I²C devices in parallel:

| Device pin | Connect to ESP32 DevKit / WROOM |
|---|---|
| OLED `VCC` | `3V3` |
| OLED `GND` | `GND` |
| OLED `SDA` | `GPIO21` |
| OLED `SCL` | `GPIO22` |
| MPU `VCC` | `3V3` |
| MPU `GND` | `GND` |
| MPU `SDA` | `GPIO21` |
| MPU `SCL` | `GPIO22` |
| Optional touch input | `T0` / `GPIO4` |

The touch input is not an OLED connection. It is enabled only by the classic ESP32 profile; ESP32-C3 does not use the built-in touch input in this firmware.

## Bus and address details

The firmware calls `Wire.begin(SDA, SCL)` and sets the I²C clock to 400 kHz. Expected devices from the firmware's I²C scan are:

| I²C address | Device |
|---|---|
| `0x3C` | SSD1306 OLED |
| `0x68` | MPU6050/MPU6500 motion sensor |

Both devices may remain on the same two-wire bus because they have different addresses. If the motion module has an address-selection jumper, leave it at `0x68` for the default firmware configuration. The OLED reset pin is not separately wired; the firmware uses `OLED_RESET = -1`.

Most breakout boards include I²C pull-up resistors. If an assembled bus is unreliable, check that SDA and SCL are not shorted, keep the wires short, and ensure the bus has suitable pull-ups to 3.3 V. Avoid stacking multiple strong pull-up networks without checking the resulting resistance.

## Build and verification

1. Disconnect USB power while making or changing connections.
2. Connect `3V3` and `GND` first, then connect SDA and SCL.
3. Inspect for reversed `VCC`/`GND` and accidental 5 V connections.
4. Upload the firmware for the board profile matching the hardware.
5. Open the serial monitor at **115200 baud**. At boot, the firmware prints the selected board, SDA/SCL GPIOs, and I²C addresses.
6. Confirm an I²C scan reports `0x3C` (OLED) and, when the motion sensor is fitted, `0x68` (MPU).

If the OLED is missing, first verify that its four-pin order is read from the labels on the particular module—connector order is not universal—and then verify the selected board profile's SDA/SCL GPIOs.

## Custom pin assignment

For another ESP32 board or a different wiring layout, select `YETI_BOARD_CUSTOM` and change `YETI_I2C_SDA_PIN` and `YETI_I2C_SCL_PIN` in the firmware. Keep the OLED and MPU on the same SDA/SCL pair, and update the wiring table for the chosen board before building.
