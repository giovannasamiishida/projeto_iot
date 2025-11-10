#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <math.h>

const int LED_WIFI   = 2; 
const int LED_MQTT   = 15;  
const int BUZZER_PIN = 12; 
const uint8_t MPU_ADDR = 0x68; 
const unsigned long CAL_MS          = 2000;
const unsigned long START_IGNORE_MS = 3000;
const unsigned long ALARM_MS        = 1500;
const unsigned long POSTURE_CONFIRM_MS = 1500;   
const unsigned long POSTURE_MAX_MS     = 3000;   
const unsigned long IMP_COOLDOWN_MS    = 2000;   
float g0 = 1.0f;                                 
float IMP_THRESH_G_REL = 2.2f;                 
float Z_TILT_THRESH_G  = 0.35f;                 

const char* ssid             = "Wokwi-GUEST";
const char* password         = "";
const char* mqtt_server      = "test.mosquitto.org";
const char* mqtt_client_id   = "WOKWI_Client";
const char* mqtt_topic_alert = "iot/fall/alert";

WiFiClient WOKWI_Client;
PubSubClient client(WOKWI_Client);

unsigned long bootMs         = 0;
unsigned long alarmStartMs   = 0;
bool alarming = false;

bool   impactPending   = false; 
unsigned long impactMs = 0;       
unsigned long lastImpMs= 0;     

int16_t read16(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, (uint8_t)2, (uint8_t)true);
  int16_t hi = Wire.read();
  int16_t lo = Wire.read();
  return (hi << 8) | lo;
}

void mpuInit() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x6B); Wire.write(0x00); Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x1C); Wire.write(0x00); Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR); Wire.write(0x1B); Wire.write(0x00); Wire.endTransmission(true);
}

void readAccelAxes(float& ax_g, float& ay_g, float& az_g) {
  int16_t ax = read16(0x3B);
  int16_t ay = read16(0x3D);
  int16_t az = read16(0x3F);
  ax_g = ax / 16384.0f;
  ay_g = ay / 16384.0f;
  az_g = az / 16384.0f;
}

float readAccelMag() {
  float ax_g, ay_g, az_g;
  readAccelAxes(ax_g, ay_g, az_g);
  return sqrtf(ax_g*ax_g + ay_g*ay_g + az_g*az_g);
}

void setup_wifi() {
  delay(10);
  Serial.println(); Serial.print("Connecting to "); Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    digitalWrite(LED_WIFI, !digitalRead(LED_WIFI));
  }
  Serial.println(); Serial.println("WiFi connected");
  Serial.print("IP address: "); Serial.println(WiFi.localIP());
  digitalWrite(LED_WIFI, HIGH);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      digitalWrite(LED_MQTT, HIGH);
      client.publish("iot/fall/status", "online");
    } else {
      Serial.print("failed, rc="); Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      digitalWrite(LED_MQTT, LOW);
      delay(5000);
    }
  }
}

void Conectado_Wifi()      { digitalWrite(LED_WIFI,  WiFi.status()==WL_CONNECTED ? HIGH : LOW); }
void Conectado_Mosquitto() { digitalWrite(LED_MQTT, client.connected()          ? HIGH : LOW);  }

void alarmOn()  { tone(BUZZER_PIN, 1000); }
void alarmOff() { noTone(BUZZER_PIN); }

void publishEvent(const char* kind, float a_mag, unsigned long ms, float az=0.0f) {
  if (!client.connected()) return;
  char payload[200];
  snprintf(payload, sizeof(payload),
           "{\"event\":\"%s\",\"a_mag\":%.2f,\"az\":%.2f,\"ms\":%lu}", kind, a_mag, az, ms);
  client.publish(mqtt_topic_alert, payload);
}

void calibrateG0() {
  Serial.println("Calibrating MPU6050...");
  delay(CAL_MS);
  const int N = 100;
  double sum = 0.0;
  for (int i = 0; i < N; i++) {
    sum += readAccelMag();
    delay(5);
  }
  g0 = sum / N;
  if (g0 < 0.5f || g0 > 1.5f) g0 = 1.0f;  
  Serial.print("g0 calibrated = "); Serial.println(g0, 3);
}

void setup() {
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_MQTT, OUTPUT);
  Serial.begin(115200);
  mpuInit();
  bootMs = millis();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  calibrateG0();

  tone(BUZZER_PIN, 1800); delay(120); noTone(BUZZER_PIN); delay(120);
  tone(BUZZER_PIN, 2200); delay(120); noTone(BUZZER_PIN);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  Conectado_Wifi();
  Conectado_Mosquitto();

  unsigned long now = millis();
  if (now - bootMs < START_IGNORE_MS) return;

  float ax, ay, az;
  readAccelAxes(ax, ay, az);
  float a_mag = sqrtf(ax*ax + ay*ay + az*az);

  static float a_mag_f = 1.0f;
  a_mag_f = 0.9f * a_mag_f + 0.1f * a_mag;
  a_mag   = a_mag_f;

  const float impThresh = IMP_THRESH_G_REL * g0;
  if (!impactPending && (now - lastImpMs) > IMP_COOLDOWN_MS && a_mag > impThresh) {
    impactPending = true;
    impactMs = now;
    lastImpMs = now;
    Serial.print("[IMPACTO] |a|="); Serial.println(a_mag, 3);
    publishEvent("impact_detected", a_mag, now, az);
  }

  if (impactPending) {
    unsigned long dt = now - impactMs;

    if (dt >= POSTURE_CONFIRM_MS && dt <= POSTURE_MAX_MS) {
      if (fabs(az) < Z_TILT_THRESH_G) {
        Serial.println("[QUEDA] impacto seguido de inclinacao em Z (horizontal).");
        publishEvent("fall_confirmed_impact_then_tiltZ", a_mag, now, az);
        alarming = true;
        alarmStartMs = now;
        alarmOn();
        impactPending = false;
      }
    }

    if (dt > POSTURE_MAX_MS) {
      Serial.println("[DESCARTE] sem inclinacao Z suficiente apos impacto.");
      publishEvent("discard_no_tiltZ", a_mag, now, az);
      impactPending = false;
    }
  }

  if (alarming && (now - alarmStartMs) > ALARM_MS) {
    alarming = false;
    alarmOff();
  }

  delay(20);
}
