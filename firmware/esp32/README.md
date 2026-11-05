# ESP32 LED + MG996R Servo Firmware

Arduino firmware for an ESP32 DevKit with three independently controlled LEDs and one MG996R servo.

## Hardware

| Component | ESP32 pin / supply |
|---|---|
| LED 1 anode (+), through 220 Ω resistor | GPIO 25 |
| LED 2 anode (+), through 220 Ω resistor | GPIO 26 |
| LED 3 anode (+), through 220 Ω resistor | GPIO 27 |
| LED cathodes (-) | GND |
| MG996R signal (orange) | GPIO 18 |
| MG996R VCC (red) | External regulated 5–6 V supply |
| MG996R GND (brown) | External supply GND |
| ESP32 GND | External supply GND |

**Power warning:** Do not power the MG996R from an ESP32 GPIO or the board's 3.3 V output. Use a separate supply capable of handling the servo's high peak/stall current. Connect the external supply ground and ESP32 ground together. Keep the servo mechanically unloaded during initial testing.

## Requirements

- ESP32 Arduino core
- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) library

Install ESP32Servo through Arduino IDE Library Manager.

## Upload

1. Open `esp32_led_servo.ino` in Arduino IDE.
2. Select your ESP32 board and serial port.
3. Install ESP32Servo.
4. Upload the sketch.
5. Open Serial Monitor at **115200 baud**, with **Newline** line ending.

## Serial commands

| Command | Action |
|---|---|
| `help` | Show commands |
| `led1 on` / `led1 off` | LED 1 control |
| `led2 on` / `led2 off` | LED 2 control |
| `led3 on` / `led3 off` | LED 3 control |
| `led all on` / `led all off` | Control all LEDs |
| `servo 0` ... `servo 180` | Set servo angle |
| `status` | Show current output states |

All LEDs are initialized OFF. The servo is commanded to 90 degrees at startup. Servo angle is a commanded position, not feedback from a position sensor.
