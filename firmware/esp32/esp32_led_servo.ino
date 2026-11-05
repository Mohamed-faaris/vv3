/*
 * ESP32 LED + MG996R Servo Controller
 *
 * Board: ESP32 DevKit
 * LEDs: GPIO 25, 26, 27 (active HIGH)
 * Servo signal: GPIO 18
 *
 * Serial baud: 115200
 * Line ending: Newline
 *
 * Commands:
 *   help
 *   led1 on|off
 *   led2 on|off
 *   led3 on|off
 *   led all on|off
 *   servo <0-180>
 *   status
 *
 * Install library: ESP32Servo by Kevin Harrington / John K. Bennett.
 */

#include <Arduino.h>
#include <ESP32Servo.h>

namespace Pins {
constexpr uint8_t LED1 = 25;
constexpr uint8_t LED2 = 26;
constexpr uint8_t LED3 = 27;
constexpr uint8_t SERVO = 18;
}

namespace Settings {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr int SERVO_MIN_US = 500;
constexpr int SERVO_MAX_US = 2500;
constexpr int SERVO_START_ANGLE = 90;
constexpr size_t COMMAND_BUFFER_SIZE = 64;
}

Servo servo;
char commandBuffer[Settings::COMMAND_BUFFER_SIZE];
size_t commandLength = 0;
int currentServoAngle = Settings::SERVO_START_ANGLE;

void printHelp() {
  Serial.println(F("\nCommands:"));
  Serial.println(F("  help           Show commands"));
  Serial.println(F("  led1 on|off    Control LED 1 (GPIO 25)"));
  Serial.println(F("  led2 on|off    Control LED 2 (GPIO 26)"));
  Serial.println(F("  led3 on|off    Control LED 3 (GPIO 27)"));
  Serial.println(F("  led all on|off Control all LEDs"));
  Serial.println(F("  servo <0-180>  Set servo angle"));
  Serial.println(F("  status         Show output states"));
}

void setLed(uint8_t pin, bool enabled) {
  digitalWrite(pin, enabled ? HIGH : LOW);
}

void printStatus() {
  Serial.printf(
      "LED1=%s LED2=%s LED3=%s SERVO=%d\n",
      digitalRead(Pins::LED1) ? "ON" : "OFF",
      digitalRead(Pins::LED2) ? "ON" : "OFF",
      digitalRead(Pins::LED3) ? "ON" : "OFF",
      currentServoAngle);
}

void processCommand(char *command) {
  // Trim leading spaces.
  while (*command == ' ' || *command == '\t') {
    ++command;
  }

  // Trim trailing spaces.
  size_t length = strlen(command);
  while (length > 0 &&
         (command[length - 1] == ' ' || command[length - 1] == '\t' ||
          command[length - 1] == '\r')) {
    command[--length] = '\0';
  }

  // Convert to lowercase.
  for (char *p = command; *p; ++p) {
    *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
  }

  if (length == 0) return;

  if (strcmp(command, "help") == 0) {
    printHelp();
  } else if (strcmp(command, "led1 on") == 0) {
    setLed(Pins::LED1, true);
    Serial.println(F("LED1 ON"));
  } else if (strcmp(command, "led1 off") == 0) {
    setLed(Pins::LED1, false);
    Serial.println(F("LED1 OFF"));
  } else if (strcmp(command, "led2 on") == 0) {
    setLed(Pins::LED2, true);
    Serial.println(F("LED2 ON"));
  } else if (strcmp(command, "led2 off") == 0) {
    setLed(Pins::LED2, false);
    Serial.println(F("LED2 OFF"));
  } else if (strcmp(command, "led3 on") == 0) {
    setLed(Pins::LED3, true);
    Serial.println(F("LED3 ON"));
  } else if (strcmp(command, "led3 off") == 0) {
    setLed(Pins::LED3, false);
    Serial.println(F("LED3 OFF"));
  } else if (strcmp(command, "led all on") == 0) {
    setLed(Pins::LED1, true);
    setLed(Pins::LED2, true);
    setLed(Pins::LED3, true);
    Serial.println(F("ALL LEDS ON"));
  } else if (strcmp(command, "led all off") == 0) {
    setLed(Pins::LED1, false);
    setLed(Pins::LED2, false);
    setLed(Pins::LED3, false);
    Serial.println(F("ALL LEDS OFF"));
  } else if (strncmp(command, "servo ", 6) == 0) {
    char *value = command + 6;
    char *end = nullptr;
    long angle = strtol(value, &end, 10);

    while (end && (*end == ' ' || *end == '\t')) ++end;

    if (value == end || (end && *end != '\0') || angle < 0 || angle > 180) {
      Serial.println(F("ERROR: servo angle must be an integer from 0 to 180"));
      return;
    }

    currentServoAngle = static_cast<int>(angle);
    servo.write(currentServoAngle);
    Serial.printf("SERVO %d\n", currentServoAngle);
  } else if (strcmp(command, "status") == 0) {
    printStatus();
  } else {
    Serial.println(F("ERROR: unknown command. Type help."));
  }
}

void readSerialCommands() {
  while (Serial.available() > 0) {
    const char incoming = static_cast<char>(Serial.read());

    if (incoming == '\n') {
      commandBuffer[commandLength] = '\0';
      processCommand(commandBuffer);
      commandLength = 0;
    } else if (incoming != '\r') {
      if (commandLength < Settings::COMMAND_BUFFER_SIZE - 1) {
        commandBuffer[commandLength++] = incoming;
      } else {
        commandLength = 0;
        Serial.println(F("ERROR: command too long; buffer cleared"));
      }
    }
  }
}

void setup() {
  Serial.begin(Settings::SERIAL_BAUD);

  pinMode(Pins::LED1, OUTPUT);
  pinMode(Pins::LED2, OUTPUT);
  pinMode(Pins::LED3, OUTPUT);

  // Fail-safe LED state: all OFF.
  setLed(Pins::LED1, false);
  setLed(Pins::LED2, false);
  setLed(Pins::LED3, false);

  servo.setPeriodHertz(50);
  servo.attach(Pins::SERVO, Settings::SERVO_MIN_US, Settings::SERVO_MAX_US);
  if (servo.attached()) {
    servo.write(Settings::SERVO_START_ANGLE);
  } else {
    Serial.println(F("ERROR: servo attach failed"));
  }

  currentServoAngle = Settings::SERVO_START_ANGLE;

  Serial.println(F("\nESP32 LED + MG996R CONTROLLER READY"));
  printHelp();
}

void loop() {
  readSerialCommands();
}
