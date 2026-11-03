# Wiring reference

The pin assignments below are taken from the supplied firmware. Confirm every connection against your exact ESP32 board and RA-02 breakout before powering it.

## ESP32 ↔ SX1278 / RA-02

| SX1278 / RA-02 pin | ESP32 GPIO | Firmware constant |
|---|---:|---|
| SCK | 18 | `SCK_PIN` |
| MISO | 19 | `MISO_PIN` |
| MOSI | 23 | `MOSI_PIN` |
| NSS / CS | 5 | `SS_PIN` |
| RESET | 14 | `RST_PIN` |
| DIO0 | 26 | `DIO0_PIN` |
| VCC | regulated 3.3 V | — |
| GND | GND | — |

**Important:** DIO0 is set to GPIO 26 in this version. If your actual wire is connected to GPIO 2/D2, either move the wire to GPIO 26 or deliberately change `DIO0_PIN` in all three sketches to match the verified wiring. Do not guess.

## Buttons and LEDs

| Component | ESP32 connection | Notes |
|---|---|---|
| SOS push button | GPIO 4 to GND | Firmware uses `INPUT_PULLUP`; button press reads LOW |
| Green LED | GPIO 12 | New-message indicator; use a series resistor |
| Red LED | GPIO 13 | SOS indicator; use a series resistor |

## Electrical cautions

- RA-02/SX1278 logic and supply are 3.3 V. Never feed 5 V into its VCC or signal pins.
- Use a stable 3.3 V supply with adequate peak current; connect ESP32 and LoRa grounds together.
- Connect a suitable antenna before transmitting. Do not transmit with the radio antenna port left unterminated.
- ESP32 GPIO12 is a strapping pin on many ESP32 variants. External LED wiring can affect boot on some boards; if boot problems occur, review the board's strapping-pin requirements and choose a safer GPIO consistently in the code and wiring.
- Check the legal radio frequency and transmit-power requirements for your region before operating the equipment.
