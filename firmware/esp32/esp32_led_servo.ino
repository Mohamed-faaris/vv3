/*
 * ESP32 LED + MG996R Servo Controller with Web Dashboard
 *
 * Board: ESP32 DevKit
 * LEDs: GPIO 25, 26, 27 (active HIGH)
 * Servo signal: GPIO 18
 *
 * Wi-Fi AP: ESP32-Light-Servo
 * AP password: lightservo26 (change before deployment)
 * Dashboard: http://192.168.4.1
 *
 * Serial baud: 115200, line ending: Newline
 * Libraries: ESP32Servo (third-party); WiFi, WebServer, Preferences (ESP32 core)
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <ESP32Servo.h>

namespace Pins {
constexpr uint8_t LEDS[] = {25, 26, 27};
constexpr uint8_t SERVO = 18;
}

namespace Settings {
constexpr uint32_t SERIAL_BAUD = 115200;
constexpr int SERVO_MIN_US = 500;
constexpr int SERVO_MAX_US = 2500;
constexpr int SERVO_HOME_ANGLE = 90;
constexpr size_t COMMAND_BUFFER_SIZE = 64;
constexpr char AP_SSID[] = "ESP32-Light-Servo";
constexpr char AP_PASSWORD[] = "lightservo26"; // Change before deployment.
}

Servo servo;
WebServer server(80);
Preferences preferences;

char commandBuffer[Settings::COMMAND_BUFFER_SIZE];
size_t commandLength = 0;

bool ledStates[3] = {false, false, false};
int ledServoAngles[3] = {30, 90, 150};
int currentServoAngle = Settings::SERVO_HOME_ANGLE;
int activeServoLed = -1;

const char DASHBOARD_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<meta name="theme-color" content="#0b1020">
<title>ESP32 Light Control</title>
<style>
:root{color-scheme:dark;--bg:#0b1020;--panel:#121a2d;--line:#26334d;--muted:#91a0ba;--text:#edf2ff;--accent:#7c9cff;--green:#43d19e;--red:#ff7185}
*{box-sizing:border-box}body{margin:0;background:radial-gradient(ellipse at top,#172442 0,var(--bg) 55%);color:var(--text);font:15px/1.5 Inter,ui-sans-serif,system-ui,sans-serif;min-height:100vh}
main{max-width:980px;margin:auto;padding:32px 20px 56px}.top{display:flex;justify-content:space-between;align-items:center;gap:16px;margin-bottom:28px}.eyebrow{color:var(--accent);font-size:12px;font-weight:700;letter-spacing:.16em;text-transform:uppercase}h1{font-size:clamp(26px,5vw,38px);margin:5px 0 4px;letter-spacing:-.04em}.sub{color:var(--muted);margin:0}.status{display:flex;align-items:center;gap:8px;border:1px solid var(--line);padding:9px 12px;border-radius:999px;color:var(--muted);font-size:13px;white-space:nowrap}.dot{width:8px;height:8px;border-radius:50%;background:var(--green);box-shadow:0 0 12px #43d19e66}.dot.off{background:var(--red);box-shadow:none}
.grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:14px}.card,.servo-card{background:linear-gradient(145deg,#172239,#11192b);border:1px solid var(--line);border-radius:18px;padding:20px;box-shadow:0 16px 40px #0002}.card-head{display:flex;align-items:center;justify-content:space-between;gap:10px}.light-icon{width:44px;height:44px;border-radius:13px;background:#26334d;display:grid;place-items:center;font-size:22px;filter:grayscale(1);opacity:.65}.light-icon.on{background:#f5c45122;filter:none;opacity:1;box-shadow:0 0 28px #f5c45122}.pill{font-size:11px;font-weight:700;letter-spacing:.08em;border-radius:99px;padding:5px 9px;background:#26334d;color:var(--muted)}.pill.on{background:#123d34;color:var(--green)}h2{font-size:18px;margin:16px 0 2px}.pin{font-size:12px;color:var(--muted)}.toggle{width:100%;border:0;border-radius:11px;padding:11px 14px;margin-top:18px;font-weight:750;font-size:14px;cursor:pointer;background:#26334d;color:var(--text);transition:.15s}.toggle.on{background:var(--green);color:#06271d}.toggle:disabled,button:disabled{opacity:.5;cursor:wait}.angle{margin-top:17px;padding-top:14px;border-top:1px solid var(--line)}.angle-row{display:flex;justify-content:space-between;align-items:center;font-size:13px}.angle-row strong{color:#c7d5ff;font-variant-numeric:tabular-nums}input[type=range]{width:100%;accent-color:var(--accent);margin:12px 0 4px}.save{background:transparent;border:1px solid var(--line);border-radius:8px;padding:7px 10px;color:#c7d5ff;font-size:12px;cursor:pointer}.servo-card{margin-top:16px;display:grid;grid-template-columns:1fr auto;gap:20px;align-items:center}.servo-card h2{margin:0 0 3px}.servo-card p{margin:0;color:var(--muted);font-size:13px}.servo-control{min-width:260px}.servo-value{font-size:26px;font-weight:750;font-variant-numeric:tabular-nums;text-align:right}.servo-actions{display:flex;justify-content:flex-end;gap:8px}.primary,.secondary{border:0;border-radius:9px;padding:10px 14px;font-weight:700;cursor:pointer}.primary{background:var(--accent);color:#081126}.secondary{background:#26334d;color:var(--text)}.footer{margin-top:18px;color:var(--muted);font-size:12px;display:flex;justify-content:space-between;gap:12px;flex-wrap:wrap}.notice{min-height:20px;color:#c7d5ff;font-size:13px;margin-top:12px}
@media(max-width:700px){main{padding:24px 14px}.grid{grid-template-columns:1fr}.card{padding:17px}.servo-card{grid-template-columns:1fr}.servo-control{min-width:0}.top{align-items:flex-start;flex-direction:column}.status{align-self:flex-start}}
</style>
</head>
<body>
<main>
<header class="top"><div><div class="eyebrow">ESP32 · Device dashboard</div><h1>Light control</h1><p class="sub">Switch each light and assign the servo position it triggers.</p></div><div class="status"><span id="connectionDot" class="dot"></span><span id="connectionText">Connecting…</span></div></header>
<section class="grid" id="lights">
<article class="card" data-id="1"><div class="card-head"><div class="light-icon" id="icon1">☼</div><span class="pill" id="state1">OFF</span></div><h2>Light 01</h2><div class="pin">GPIO 25</div><button class="toggle" id="toggle1" onclick="toggleLight(1)">Turn on</button><div class="angle"><div class="angle-row"><label for="angle1">Servo target</label><strong><span id="angleValue1">30</span>°</strong></div><input id="angle1" type="range" min="0" max="180" value="30" oninput="showAngle(1)"><button class="save" onclick="saveAngle(1)">Save angle</button></div></article>
<article class="card" data-id="2"><div class="card-head"><div class="light-icon" id="icon2">☼</div><span class="pill" id="state2">OFF</span></div><h2>Light 02</h2><div class="pin">GPIO 26</div><button class="toggle" id="toggle2" onclick="toggleLight(2)">Turn on</button><div class="angle"><div class="angle-row"><label for="angle2">Servo target</label><strong><span id="angleValue2">90</span>°</strong></div><input id="angle2" type="range" min="0" max="180" value="90" oninput="showAngle(2)"><button class="save" onclick="saveAngle(2)">Save angle</button></div></article>
<article class="card" data-id="3"><div class="card-head"><div class="light-icon" id="icon3">☼</div><span class="pill" id="state3">OFF</span></div><h2>Light 03</h2><div class="pin">GPIO 27</div><button class="toggle" id="toggle3" onclick="toggleLight(3)">Turn on</button><div class="angle"><div class="angle-row"><label for="angle3">Servo target</label><strong><span id="angleValue3">150</span>°</strong></div><input id="angle3" type="range" min="0" max="180" value="150" oninput="showAngle(3)"><button class="save" onclick="saveAngle(3)">Save angle</button></div></article>
</section>
<section class="servo-card"><div><div class="eyebrow">Manual control</div><h2>MG996R servo</h2><p>Manual movement overrides the light-triggered position until another light is switched on.</p><div class="pin" id="activeLight" style="margin-top:8px">No active light</div></div><div class="servo-control"><div class="servo-value"><span id="servoValue">90</span>°</div><input id="manualAngle" type="range" min="0" max="180" value="90" oninput="document.getElementById('servoValue').textContent=this.value"><div class="servo-actions"><button class="secondary" onclick="goHome()">Home · 90°</button><button class="primary" onclick="moveServo()">Move servo</button></div></div></section>
<div class="notice" id="notice" role="status"></div>
<footer class="footer"><span>ESP32 local network control · No cloud required</span><span>Refreshes device state every 2 seconds</span></footer>
</main>
<script>
const $=id=>document.getElementById(id);
async function request(path){const response=await fetch(path,{method:'POST'});const data=await response.json();if(!response.ok||!data.ok)throw new Error(data.error||'Request failed');return data}
function message(text){$('notice').textContent=text}
function showAngle(id){$('angleValue'+id).textContent=$('angle'+id).value}
async function toggleLight(id){const button=$('toggle'+id);button.disabled=true;try{const isOn=$('state'+id).textContent==='ON';await request('/api/led?id='+id+'&state='+(isOn?'off':'on')+'&angle='+$('angle'+id).value);await refresh();message('Light '+id+' switched '+(isOn?'off':'on')+'.')}catch(e){message(e.message)}finally{button.disabled=false}}
async function saveAngle(id){try{await request('/api/led-config?id='+id+'&angle='+$('angle'+id).value);message('Light '+id+' servo target saved.')}catch(e){message(e.message)}}
async function moveServo(){try{await request('/api/servo?angle='+$('manualAngle').value);await refresh();message('Servo moved to '+$('manualAngle').value+'°.')}catch(e){message(e.message)}}
async function goHome(){try{await request('/api/servo?angle=90');await refresh();message('Servo moved to home position.')}catch(e){message(e.message)}}
async function refresh(){try{const r=await fetch('/api/status',{cache:'no-store'});if(!r.ok)throw new Error('Device unavailable');const s=await r.json();for(let i=1;i<=3;i++){const on=s.leds[i-1];$('state'+i).textContent=on?'ON':'OFF';$('state'+i).classList.toggle('on',on);$('toggle'+i).textContent=on?'Turn off':'Turn on';$('toggle'+i).classList.toggle('on',on);$('icon'+i).classList.toggle('on',on);$('angle'+i).value=s.angles[i-1];showAngle(i)}$('servoValue').textContent=s.servo;$('manualAngle').value=s.servo;$('activeLight').textContent=s.activeLed?('Light '+s.activeLed+' currently controls the servo'):'No light currently controls the servo';$('connectionText').textContent='ESP32 connected';$('connectionDot').classList.remove('off')}catch(e){$('connectionText').textContent='Disconnected';$('connectionDot').classList.add('off')}}
refresh();setInterval(refresh,2000);
</script>
</body>
</html>
)HTML";

void setServoAngle(int angle) {
  currentServoAngle = constrain(angle, 0, 180);
  if (servo.attached()) servo.write(currentServoAngle);
}

void setLedState(int id, bool enabled) {
  if (id < 1 || id > 3) return;
  const int index = id - 1;
  ledStates[index] = enabled;
  digitalWrite(Pins::LEDS[index], enabled ? HIGH : LOW);

  if (enabled) {
    activeServoLed = index;
    setServoAngle(ledServoAngles[index]);
  } else if (activeServoLed == index) {
    activeServoLed = -1;
    // If another light remains on, its configured angle becomes active.
    for (int i = 2; i >= 0; --i) {
      if (ledStates[i]) {
        activeServoLed = i;
        setServoAngle(ledServoAngles[i]);
        break;
      }
    }
    if (activeServoLed == -1) setServoAngle(Settings::SERVO_HOME_ANGLE);
  }
}

void saveLedAngle(int id, int angle) {
  if (id < 1 || id > 3 || angle < 0 || angle > 180) return;
  ledServoAngles[id - 1] = angle;
  preferences.putInt((String("a") + id).c_str(), angle);
  if (ledStates[id - 1] && activeServoLed == id - 1) {
    setServoAngle(angle);
  }
}

void sendJson(const String &json, int code = 200) {
  server.sendHeader("Cache-Control", "no-store");
  server.send(code, "application/json", json);
}

void handleStatus() {
  String json = "{\"ok\":true,\"leds\":[";
  for (int i = 0; i < 3; ++i) {
    if (i) json += ",";
    json += ledStates[i] ? "true" : "false";
  }
  json += "],\"angles\":[";
  for (int i = 0; i < 3; ++i) {
    if (i) json += ",";
    json += String(ledServoAngles[i]);
  }
  json += "],\"servo\":" + String(currentServoAngle);
  json += ",\"activeLed\":" + String(activeServoLed < 0 ? 0 : activeServoLed + 1) + "}";
  sendJson(json);
}

bool readIntArg(const char *name, int minValue, int maxValue, int &value) {
  if (!server.hasArg(name) || server.arg(name).length() == 0) return false;
  const String raw = server.arg(name);
  char *end = nullptr;
  long parsed = strtol(raw.c_str(), &end, 10);
  if (end == raw.c_str() || *end != '\0' || parsed < minValue || parsed > maxValue) return false;
  value = static_cast<int>(parsed);
  return true;
}

void handleLed() {
  int id, angle;
  if (!readIntArg("id", 1, 3, id) ||
      !readIntArg("angle", 0, 180, angle) ||
      !server.hasArg("state")) {
    sendJson("{\"ok\":false,\"error\":\"Expected id 1-3, angle 0-180 and state on/off\"}", 400);
    return;
  }

  const String state = server.arg("state");
  if (state != "on" && state != "off") {
    sendJson("{\"ok\":false,\"error\":\"state must be on or off\"}", 400);
    return;
  }

  saveLedAngle(id, angle);
  setLedState(id, state == "on");
  sendJson("{\"ok\":true}");
}

void handleLedConfig() {
  int id, angle;
  if (!readIntArg("id", 1, 3, id) || !readIntArg("angle", 0, 180, angle)) {
    sendJson("{\"ok\":false,\"error\":\"Expected id 1-3 and angle 0-180\"}", 400);
    return;
  }
  saveLedAngle(id, angle);
  sendJson("{\"ok\":true}");
}

void handleServo() {
  int angle;
  if (!readIntArg("angle", 0, 180, angle)) {
    sendJson("{\"ok\":false,\"error\":\"angle must be between 0 and 180\"}", 400);
    return;
  }
  activeServoLed = -1;
  setServoAngle(angle);
  sendJson("{\"ok\":true}");
}

void startDashboard() {
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(Settings::AP_SSID, Settings::AP_PASSWORD)) {
    Serial.println(F("ERROR: Wi-Fi AP startup failed"));
    return;
  }

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", DASHBOARD_HTML);
  });
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/led", HTTP_POST, handleLed);
  server.on("/api/led-config", HTTP_POST, handleLedConfig);
  server.on("/api/servo", HTTP_POST, handleServo);
  server.onNotFound([]() {
    sendJson("{\"ok\":false,\"error\":\"Not found\"}", 404);
  });
  server.begin();

  Serial.printf("Wi-Fi AP: %s\n", Settings::AP_SSID);
  Serial.printf("Dashboard: http://%s\n", WiFi.softAPIP().toString().c_str());
}

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

void printStatus() {
  Serial.printf("LED1=%s LED2=%s LED3=%s SERVO=%d ACTIVE_LED=%d\n",
      ledStates[0] ? "ON" : "OFF",
      ledStates[1] ? "ON" : "OFF",
      ledStates[2] ? "ON" : "OFF",
      currentServoAngle,
      activeServoLed < 0 ? 0 : activeServoLed + 1);
}

void processCommand(char *command) {
  while (*command == ' ' || *command == '\t') ++command;
  size_t length = strlen(command);
  while (length > 0 && (command[length - 1] == ' ' ||
         command[length - 1] == '\t' || command[length - 1] == '\r')) {
    command[--length] = '\0';
  }
  for (char *p = command; *p; ++p) {
    *p = static_cast<char>(tolower(static_cast<unsigned char>(*p)));
  }
  if (length == 0) return;

  if (strcmp(command, "help") == 0) {
    printHelp();
  } else if (strcmp(command, "led1 on") == 0) {
    setLedState(1, true);
    Serial.println(F("LED1 ON"));
  } else if (strcmp(command, "led1 off") == 0) {
    setLedState(1, false);
    Serial.println(F("LED1 OFF"));
  } else if (strcmp(command, "led2 on") == 0) {
    setLedState(2, true);
    Serial.println(F("LED2 ON"));
  } else if (strcmp(command, "led2 off") == 0) {
    setLedState(2, false);
    Serial.println(F("LED2 OFF"));
  } else if (strcmp(command, "led3 on") == 0) {
    setLedState(3, true);
    Serial.println(F("LED3 ON"));
  } else if (strcmp(command, "led3 off") == 0) {
    setLedState(3, false);
    Serial.println(F("LED3 OFF"));
  } else if (strcmp(command, "led all on") == 0) {
    for (int i = 1; i <= 3; ++i) setLedState(i, true);
    Serial.println(F("ALL LEDS ON"));
  } else if (strcmp(command, "led all off") == 0) {
    for (int i = 1; i <= 3; ++i) setLedState(i, false);
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
    activeServoLed = -1;
    setServoAngle(static_cast<int>(angle));
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

  for (uint8_t pin : Pins::LEDS) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  preferences.begin("lightservo", false);
  for (int i = 0; i < 3; ++i) {
    ledServoAngles[i] = preferences.getInt((String("a") + (i + 1)).c_str(),
                                            ledServoAngles[i]);
  }

  servo.setPeriodHertz(50);
  servo.attach(Pins::SERVO, Settings::SERVO_MIN_US, Settings::SERVO_MAX_US);
  setServoAngle(Settings::SERVO_HOME_ANGLE);

  Serial.println(F("\nESP32 LED + MG996R CONTROLLER READY"));
  printHelp();
  startDashboard();
}

void loop() {
  readSerialCommands();
  server.handleClient();
}
