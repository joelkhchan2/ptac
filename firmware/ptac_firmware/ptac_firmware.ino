#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#include <time.h>
#include <esp_system.h>
#include "secrets.h"
#include "isrg_root_x1.h"

/*
  PTAC Servo MQTT Controller (ESP32)
  Topics:
    ptac/joel_a83f2/cmd/us            payload: SERVO_MIN_US..SERVO_MAX_US
    ptac/joel_a83f2/cmd/save/<MODE>   payload: SERVO_MIN_US..SERVO_MAX_US
    ptac/joel_a83f2/cmd/go/<MODE>     payload: anything
    ptac/joel_a83f2/state             retained {"us":N,"reason":"..."}
    ptac/joel_a83f2/status            retained "online" / "offline" (last will)
    ptac/joel_a83f2/diag              retained diagnostics: uptime, rssi, heap, last events
  Modes: OFF, HEAT3, HEAT2, COOL3, COOL2.
  The servo is powered only while moving plus HOLD_MS, never longer than
  MAX_ATTACH_MS in a row, followed by COOLDOWN_MS in which moves are refused.
*/

const char* TOPIC_CMD_PREFIX = "ptac/joel_a83f2/cmd/";
const char* TOPIC_CMD_ALL    = "ptac/joel_a83f2/cmd/#";
const char* TOPIC_US         = "ptac/joel_a83f2/cmd/us";
const char* TOPIC_SAVE       = "ptac/joel_a83f2/cmd/save/";
const char* TOPIC_GO         = "ptac/joel_a83f2/cmd/go/";
const char* TOPIC_STATE      = "ptac/joel_a83f2/state";
const char* TOPIC_STATUS     = "ptac/joel_a83f2/status";
const char* TOPIC_DIAG       = "ptac/joel_a83f2/diag";

const char* PRESETS[] = {"OFF", "HEAT3", "HEAT2", "COOL3", "COOL2"};

const int SERVO_PIN    = 13;
// Bench-test the real usable range of the servo/knob and narrow these (and the UI mapping) to match.
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;

const unsigned long HOLD_MS          = 1000;
const unsigned long MAX_ATTACH_MS    = 6000;
const unsigned long COOLDOWN_MS      = 6000;
const unsigned long CMD_ARM_DELAY_MS = 2000;
const unsigned long MQTT_RETRY_MS    = 3000;
const unsigned long WIFI_RETRY_MS    = 5000;
const unsigned long WIFI_RESTART_MS  = 60000;

const char* NS_PRESETS = "ptac";
const char* NS_STATE   = "ptac_state";
const char* KEY_LAST   = "last_us";

Servo servo;
Preferences prefs;
WiFiClientSecure tls;
PubSubClient mqtt(tls);

int currentUs = 1500;
bool positionDirty = false;
unsigned long releaseAt = 0;
unsigned long attachedSince = 0;
unsigned long cooldownUntil = 0;
unsigned long armAt = 0;
unsigned long nextMqttAttempt = 0;
unsigned long nextWifiAttempt = 0;
unsigned long wifiLostAt = 0;

const int EV_MAX = 8;
String events[EV_MAX];
int eventCount = 0;
unsigned long lastDiagAt = 0;
bool diagDirty = false;

String resetReasonName() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "poweron";
    case ESP_RST_SW:        return "software";
    case ESP_RST_PANIC:     return "panic";
    case ESP_RST_INT_WDT:   return "int_wdt";
    case ESP_RST_TASK_WDT:  return "task_wdt";
    case ESP_RST_WDT:       return "wdt";
    case ESP_RST_BROWNOUT:  return "brownout";
    case ESP_RST_DEEPSLEEP: return "deepsleep";
    default:                return "other";
  }
}

void publishDiag(bool force) {
  if (!mqtt.connected()) return;
  if (!force && millis() - lastDiagAt < 1000) {
    diagDirty = true;
    return;
  }
  lastDiagAt = millis();
  diagDirty = false;
  String j = String("{\"up\":") + (millis() / 1000) + ",\"rssi\":" + WiFi.RSSI() + ",\"heap\":" + ESP.getFreeHeap() + ",\"ev\":[";
  for (int i = 0; i < eventCount; i++) {
    if (i) j += ",";
    j += "\"" + events[i] + "\"";
  }
  j += "]}";
  mqtt.publish(TOPIC_DIAG, j.c_str(), true);
}

void logEvent(const String& e, bool force = false) {
  String line = String(millis() / 1000) + "s " + e;
  Serial.println("EV " + line);
  if (eventCount == EV_MAX) {
    for (int i = 1; i < EV_MAX; i++) events[i - 1] = events[i];
    eventCount--;
  }
  events[eventCount++] = line;
  publishDiag(force);
}

bool reached(unsigned long t) {
  return (long)(millis() - t) >= 0;
}

bool parseUs(const String& s, int& out) {
  if (s.length() == 0 || s.length() > 4) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  out = s.toInt();
  return out >= SERVO_MIN_US && out <= SERVO_MAX_US;
}

bool isPreset(const String& s) {
  for (const char* p : PRESETS) {
    if (s == p) return true;
  }
  return false;
}

void releaseServo(bool forced = false) {
  if (servo.attached()) {
    logEvent(forced ? "release forced" : "release");
    servo.detach();
    pinMode(SERVO_PIN, OUTPUT);
    digitalWrite(SERVO_PIN, LOW);
  }
  if (positionDirty) {
    if (prefs.begin(NS_STATE, false)) {
      prefs.putInt(KEY_LAST, currentUs);
      prefs.end();
    }
    positionDirty = false;
  }
}

void releaseIfIdle() {
  if (!servo.attached()) return;
  if (reached(releaseAt)) {
    releaseServo();
  } else if (millis() - attachedSince >= MAX_ATTACH_MS) {
    releaseServo(true);
    cooldownUntil = millis() + COOLDOWN_MS;
  }
}

bool moveToUs(int us) {
  if (!servo.attached() && !reached(cooldownUntil)) return false;
  currentUs = us;
  if (!servo.attached()) {
    servo.setPeriodHertz(50);
    servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
    attachedSince = millis();
    logEvent(String("attach us=") + currentUs);
  }
  servo.writeMicroseconds(currentUs);
  releaseAt = millis() + HOLD_MS;
  positionDirty = true;
  return true;
}

void publishState(const String& reason) {
  logEvent("state " + reason + " us=" + currentUs);
  String msg = String("{\"us\":") + currentUs + ",\"reason\":\"" + reason + "\"}";
  mqtt.publish(TOPIC_STATE, msg.c_str(), true);
}

String payloadToString(byte* payload, unsigned int len) {
  String s;
  s.reserve(len);
  for (unsigned int i = 0; i < len; i++) s += (char)payload[i];
  s.trim();
  return s;
}

void onMessage(char* topic, byte* payload, unsigned int len) {
  String t = String(topic);
  String msg = payloadToString(payload, len);

  Serial.print("RX topic=");
  Serial.print(t);
  Serial.print(" payload=");
  Serial.println(msg);

  // Retained commands are delivered right after subscribing; ignore them so a stale command is never replayed.
  if (!reached(armAt)) return;
  if (!t.startsWith(TOPIC_CMD_PREFIX)) return;

  if (t == TOPIC_US) {
    int us;
    if (!parseUs(msg, us)) {
      publishState("rejected_us");
      return;
    }
    publishState(moveToUs(us) ? "us" : "rate_limited");
    return;
  }

  if (t.startsWith(TOPIC_SAVE)) {
    String name = t.substring(strlen(TOPIC_SAVE));
    int us;
    if (!isPreset(name) || !parseUs(msg, us)) {
      publishState("rejected_save");
      return;
    }
    bool stored = false;
    if (prefs.begin(NS_PRESETS, false)) {
      stored = prefs.putInt(name.c_str(), us) == sizeof(int);
      prefs.end();
    }
    if (!stored) {
      publishState("save_failed_" + name);
      return;
    }
    moveToUs(us);
    publishState("save_" + name);
    return;
  }

  if (t.startsWith(TOPIC_GO)) {
    String name = t.substring(strlen(TOPIC_GO));
    if (!isPreset(name)) {
      publishState("rejected_go");
      return;
    }
    int us = -1;
    if (prefs.begin(NS_PRESETS, true)) {
      if (prefs.isKey(name.c_str())) us = prefs.getInt(name.c_str(), -1);
      prefs.end();
    }
    if (us < SERVO_MIN_US || us > SERVO_MAX_US) {
      publishState("unknown_" + name);
      return;
    }
    publishState(moveToUs(us) ? "go_" + name : String("rate_limited"));
    return;
  }
}

void connectWiFiAtBoot() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    if (millis() - start > 30000) {
      Serial.println("\nWiFi timeout, restarting...");
      ESP.restart();
    }
  }
  Serial.println();
  Serial.print("WiFi connected. IP=");
  Serial.println(WiFi.localIP());
}

void maintainWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    wifiLostAt = 0;
    return;
  }
  if (wifiLostAt == 0) {
    wifiLostAt = millis();
    logEvent("wifi_lost");
  }
  if (reached(nextWifiAttempt)) {
    nextWifiAttempt = millis() + WIFI_RETRY_MS;
    WiFi.reconnect();
  }
  if (millis() - wifiLostAt > WIFI_RESTART_MS) {
    Serial.println("WiFi down too long, restarting...");
    releaseServo();
    ESP.restart();
  }
}

bool timeSynced() {
  return time(nullptr) > 1700000000;
}

void tryConnectMQTT() {
  if (!timeSynced()) {
    Serial.println("Waiting for NTP time (needed for TLS certificate check)");
    return;
  }

  String clientId = "esp32-ptac-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  Serial.print("MQTT connecting as ");
  Serial.print(clientId);
  Serial.print(" ... ");

  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS, TOPIC_STATUS, 0, true, "offline")) {
    Serial.println("connected");
    bool subOk = mqtt.subscribe(TOPIC_CMD_ALL);
    Serial.print("SUB ");
    Serial.print(TOPIC_CMD_ALL);
    Serial.println(subOk ? " ok" : " failed");
    armAt = millis() + CMD_ARM_DELAY_MS;
    logEvent("mqtt_connected", true);
    mqtt.publish(TOPIC_STATUS, "online", true);
    publishState("boot");
  } else {
    Serial.print("failed rc=");
    Serial.println(mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(SERVO_PIN, OUTPUT);
  digitalWrite(SERVO_PIN, LOW);

  if (prefs.begin(NS_STATE, true)) {
    currentUs = prefs.getInt(KEY_LAST, 1500);
    prefs.end();
  }
  if (currentUs < SERVO_MIN_US || currentUs > SERVO_MAX_US) currentUs = 1500;

  logEvent("boot rst=" + resetReasonName());
  connectWiFiAtBoot();

  configTime(0, 0, "pool.ntp.org", "time.google.com");

  tls.setCACert(ISRG_ROOT_X1);
  tls.setTimeout(5000);
  tls.setHandshakeTimeout(8);

  mqtt.setBufferSize(1024);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);

  Serial.println("READY");
}

void loop() {
  releaseIfIdle();
  maintainWiFi();

  if (WiFi.status() != WL_CONNECTED) return;

  if (mqtt.connected()) {
    mqtt.loop();
    if (diagDirty) publishDiag(false);
  } else if (!servo.attached() && reached(nextMqttAttempt)) {
    // A connect attempt can block for several seconds; never do it while the servo is powered.
    nextMqttAttempt = millis() + MQTT_RETRY_MS;
    tryConnectMQTT();
  }
}
