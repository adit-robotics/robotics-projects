# ESP32 Desk Buddy

Cute ESP32 desk companion with animated eyes, touch controls, NTP time, and Open-Meteo weather.

## Hardware

### TFT (1.8" ST7735)

| TFT | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| CS | GPIO 5 |
| RESET | GPIO 17 |
| A0 / DC | GPIO 16 |
| SDA | GPIO 23 |
| SCK | GPIO 18 |
| LED | 3V3 |

`SDA` on this SPI TFT is the data/MOSI connection.

### Touch sensor

| Touch | ESP32 |
|---|---|
| VCC | 3V3 |
| GND | GND |
| I/O | GPIO 32 |

## Software

Install:
- Adafruit GFX Library
- Adafruit ST7735 and ST7789 Library
- ArduinoJson

Board: **ESP32 Dev Module**

## Setup

Open `src/ESP32_Desk_Buddy.ino` and replace:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

Set your coordinates:

```cpp
const float LATITUDE = 18.5204;
const float LONGITUDE = 73.8567;
```

The example coordinates are Pune, India.

## Modes

Touch cycles:

**Eyes → Time → Weather → Eyes**

The TFT uses:

```cpp
tft.initR(INITR_BLACKTAB);
tft.setRotation(2);
```

The eye animation only redraws the eye area, so the whole screen does not flash during movement.
