#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <ESP32Servo.h>
#include "secrets.h"

/*
  PTAC Servo MQTT Controller (ESP32)
  Topics (unchanged):
    ptac/joel_a83f2/cmd/us            payload: 500-2500 (microseconds)
    ptac/joel_a83f2/cmd/save/<MODE>   payload: 500-2500 (microseconds)
    ptac/joel_a83f2/cmd/go/<MODE>     payload: anything
    ptac/joel_a83f2/state             retained {"us":N,"reason":"..."}
  The servo is only powered (PWM attached) while moving plus HOLD_MS, then released.
*/

const char* TOPIC_CMD_PREFIX = "ptac/joel_a83f2/cmd/";
const char* TOPIC_CMD_ALL    = "ptac/joel_a83f2/cmd/#";
const char* TOPIC_US         = "ptac/joel_a83f2/cmd/us";
const char* TOPIC_SAVE       = "ptac/joel_a83f2/cmd/save/";
const char* TOPIC_GO         = "ptac/joel_a83f2/cmd/go/";
const char* TOPIC_STATE      = "ptac/joel_a83f2/state";

const int SERVO_PIN    = 13;
const int SERVO_MIN_US = 500;
const int SERVO_MAX_US = 2500;
const unsigned long HOLD_MS = 1000;

const char* NS_PRESETS = "ptac";
const char* NS_STATE   = "ptac_state";
const char* KEY_LAST   = "last_us";

Servo servo;
Preferences prefs;
WiFiClientSecure tls;
PubSubClient mqtt(tls);

int currentUs = 1500;
unsigned long releaseAt = 0;
bool positionDirty = false;

bool parseUs(const String& s, int& out) {
  if (s.length() == 0 || s.length() > 4) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    if (!isDigit(s[i])) return false;
  }
  out = s.toInt();
  return out >= SERVO_MIN_US && out <= SERVO_MAX_US;
}

bool validName(const String& s) {
  if (s.length() == 0 || s.length() > 15) return false;
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (!isAlphaNumeric(c) && c != '_') return false;
  }
  return true;
}

void releaseServo() {
  if (servo.attached()) {
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
  if (servo.attached() && (long)(millis() - releaseAt) >= 0) releaseServo();
}

void moveToUs(int us) {
  currentUs = us;
  if (!servo.attached()) {
    servo.setPeriodHertz(50);
    servo.attach(SERVO_PIN, SERVO_MIN_US, SERVO_MAX_US);
  }
  servo.writeMicroseconds(currentUs);
  releaseAt = millis() + HOLD_MS;
  positionDirty = true;
}

void publishState(const String& reason) {
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

  if (!t.startsWith(TOPIC_CMD_PREFIX)) return;

  if (t == TOPIC_US) {
    int us;
    if (!parseUs(msg, us)) {
      publishState("rejected_us");
      return;
    }
    moveToUs(us);
    publishState("us");
    return;
  }

  if (t.startsWith(TOPIC_SAVE)) {
    String name = t.substring(strlen(TOPIC_SAVE));
    int us;
    if (!validName(name) || !parseUs(msg, us)) {
      publishState("rejected_save");
      return;
    }
    if (prefs.begin(NS_PRESETS, false)) {
      prefs.putInt(name.c_str(), us);
      prefs.end();
    }
    moveToUs(us);
    publishState("save_" + name);
    return;
  }

  if (t.startsWith(TOPIC_GO)) {
    String name = t.substring(strlen(TOPIC_GO));
    if (!validName(name)) {
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
    moveToUs(us);
    publishState("go_" + name);
    return;
  }
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  Serial.print("WiFi connecting");
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
    releaseIfIdle();
    if (millis() - start > 30000) {
      Serial.println("\nWiFi timeout, restarting...");
      releaseServo();
      ESP.restart();
    }
  }
  Serial.println();
  Serial.print("WiFi connected. IP=");
  Serial.println(WiFi.localIP());
}

void connectMQTT() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMessage);

  while (!mqtt.connected()) {
    String clientId = "esp32-ptac-" + String((uint32_t)ESP.getEfuseMac(), HEX);

    Serial.print("MQTT connecting as ");
    Serial.print(clientId);
    Serial.print(" ... ");

    if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS)) {
      Serial.println("connected");
      bool subOk = mqtt.subscribe(TOPIC_CMD_ALL);
      Serial.print("SUB ");
      Serial.print(TOPIC_CMD_ALL);
      Serial.println(subOk ? " ok" : " failed");
      publishState("boot");
    } else {
      Serial.print("failed rc=");
      Serial.print(mqtt.state());
      Serial.println(" (retry in 2s)");
      for (int i = 0; i < 20; i++) {
        delay(100);
        releaseIfIdle();
      }
    }
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

  connectWiFi();

  // Switch to tls.setCACert(<root CA PEM>) for strict validation
  tls.setInsecure();
  tls.setTimeout(15000);

  connectMQTT();

  Serial.println("READY");
}

void loop() {
  releaseIfIdle();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    connectWiFi();
  }

  if (!mqtt.connected()) {
    Serial.println("MQTT disconnected, reconnecting...");
    connectMQTT();
  }

  mqtt.loop();
}
