# Dasai Mochi ESP32

An open-source implementation of the popular **Dasai Mochi** facial expression display, powered by an **ESP32** microcontroller. This project brings expressive, animated faces to life on a compact display using an ESP32 board.

---

## Features

- **Dynamic Animations:** Smooth facial expressions and emotion transitions.
- **ESP32 Powered:** Utilizes the cost-effective and powerful ESP32 Wi-Fi/Bluetooth module.
- **Customizable UI:** Easily add or modify animations and expressions.
- **Compact & Lightweight:** Designed for easy integration into custom desktop toys, props, or desk accessories.

---

## Hardware Requirements

1. **ESP32 Development Board** (e.g., NodeMCU 32S or ESP32-WROOM-32)
2. **Display Module** (e.g., TFT LCD / OLED display compatible with your setup, such as ST7789 or ILI9341)
3. **Connecting Wires / Breadboard**
4. **USB Cable** for programming and power

---

## Pinout / Wiring

| ESP32 Pin | Display Pin | Description |
| :--- | :--- | :--- |
| `GPIO 23` | MOSI (SDA) | SPI Data Out |
| `GPIO 18` | SCK (SCL) | SPI Clock |
| `GPIO 5` | CS | Chip Select |
| `GPIO 2` | DC | Data/Command |
| `GPIO 4` | RST | Reset |
| `5V / 3.3V`| VCC | Power Supply |
| `GND` | GND | Ground |

*(Note: Adjust the pin mappings above according to your specific display controller and wiring schematic.)*

---

## Software & Libraries

Make sure you have the following installed in your **Arduino IDE** (or PlatformIO):

- **ESP32 Board Support Package**
- **Adafruit GFX Library** (or equivalent display graphics library)
- **TFT_eSPI Library** (recommended for performance-optimized rendering)

---

## Getting Started

1. **Clone the Repository:**
   ```bash
   git clone [https://github.com/deepan46953-design/DASAI_MOCHI_ESP32.git](https://github.com/deepan46953-design/DASAI_MOCHI_ESP32.git)
