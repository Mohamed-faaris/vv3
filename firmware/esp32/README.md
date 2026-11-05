# ESP32 LED + MG996R Servo Controller

ESP32 firmware with a local Wi-Fi web dashboard and Serial CLI for three LEDs and one MG996R servo.

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

**Servo power warning:** Do not power the MG996R from an ESP32 GPIO, 3.3 V output, or the board's USB supply. Use a separate regulated 5–6 V supply capable of supplying the servo's peak/stall current (allow roughly 2–3 A headroom for a single MG996R). Connect external supply GND and ESP32 GND together. Keep the servo unloaded during initial testing.

## Requirements

- ESP32 Arduino core
- [ESP32Servo](https://github.com/madhephaestus/ESP32Servo) library
- ESP32 core libraries: WiFi, WebServer, Preferences (included with the ESP32 board package)

Install **ESP32Servo** through Arduino IDE Library Manager.

## Upload and open the dashboard

1. Open `esp32_led_servo.ino` in Arduino IDE.
2. Select your ESP32 board and serial port.
3. Install ESP32Servo.
4. Upload the sketch.
5. Connect your phone or computer to the ESP32 Wi-Fi network:
   - SSID: `ESP32-Light-Servo`
   - Password: `123456789`
6. Open **http://192.168.4.1** in a browser.

The ESP32 runs its own access point; an internet connection is not required. Change `Settings::AP_PASSWORD` in the sketch before deployment.

## Dashboard behavior

- Three individual light cards control GPIO 25, 26 and 27.
- Each light has a configurable servo target angle (0–180°).
- Press **Save angle** to store that light's target in ESP32 NVS. The setting survives reboot.
- Turning a light ON moves the servo to that light's configured angle.
- Turning the currently active light OFF moves to the most recently selected remaining ON light's angle. If no lights remain ON, the servo returns to its 90° home position.
- The manual servo slider and **Move servo** button directly set the servo angle. **Home** returns it to 90°.
- Dashboard status refreshes every two seconds.

The servo reports its commanded angle; there is no position feedback sensor in this wiring.

## Serial CLI

Open Serial Monitor at **115200 baud**, with **Newline** line ending.

| Command | Action |
|---|---|
| `help` | Show commands |
| `led1 on` / `led1 off` | LED 1 control |
| `led2 on` / `led2 off` | LED 2 control |
| `led3 on` / `led3 off` | LED 3 control |
| `led all on` / `led all off` | Control all LEDs |
| `servo 0` ... `servo 180` | Set servo angle |
| `status` | Show output states |

## Local HTTP API

The dashboard uses these endpoints on the ESP32:

| Method | Endpoint | Purpose |
|---|---|---|
| GET | `/api/status` | Read LED states, saved angles and servo command |
| POST | `/api/led?id=1&state=on&angle=45` | Set a light and its servo target |
| POST | `/api/led-config?id=1&angle=45` | Save a light's servo target |
| POST | `/api/servo?angle=90` | Set the servo angle manually |

LED IDs are 1–3 and angles are 0–180. Invalid values are rejected.

The API is intended for clients connected to the ESP32's password-protected local access point. It does not implement application-level authentication or HTTPS; do not expose this AP or its control endpoints to an untrusted network.

## Startup defaults

- All three LEDs start ON.
- Servo starts at the configured target angle for LED 3 (150° by default; saved NVS angle takes precedence).
- Per-light servo target defaults are 30°, 90° and 150°; saved values in NVS override these defaults.
