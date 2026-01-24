#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* WIFI_SSID = "Blazevic";
const char* WIFI_PASS = "Rezolucija18";

const char* MQTT_HOST = "192.168.100.74";
const int   MQTT_PORT = 1883;

const char* TOPIC_ARM = "cosplay/bomb/arm";

const int TRIG_PIN = 4;
const int ECHO_PIN = 5;

const int TRIGGER_CM = 10;
const unsigned long COOLDOWN_MS = 5000;

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long lastTrigger = 0;

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) delay(300);
}

void connectMQTT() {
  while (!mqtt.connected()) {
    String id = "croduino-sonar-";
    id += String((uint32_t)ESP.getChipId(), HEX);
    mqtt.connect(id.c_str());
    delay(500);
  }
}

int readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long d = pulseIn(ECHO_PIN, HIGH, 25000UL);
  if (d == 0) return -1;
  return (int)(d / 58);
}

int readDistanceCmFiltered() {
  int a[5];
  for (int i=0;i<5;i++){
    a[i] = readDistanceCm();
    delay(30);
  }
  //sort 5 brojeva za median 
  for(int i=0;i<4;i++)
    for(int j=i+1;j<5;j++)
      if(a[j]<a[i]) { int t=a[i]; a[i]=a[j]; a[j]=t; }

  int cm = a[2];
  if (cm < 2 || cm > 300) return -1;
  return cm;
}

void setup() {
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  connectWiFi();
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  connectMQTT();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWiFi();
  if (!mqtt.connected()) connectMQTT();
  mqtt.loop();

  int cm = readDistanceCmFiltered();

  if (cm > 0 && cm <= TRIGGER_CM) {
    unsigned long now = millis();
    if (now - lastTrigger > COOLDOWN_MS) {
      lastTrigger = now;
      char payload[8];
      snprintf(payload, sizeof(payload), "%d", cm);
      mqtt.publish(TOPIC_ARM, payload);
    }
  }

  delay(100);
}