#include <Arduino.h>
#include <Wire.h>
#include "max_30010_lib/MAX30100_PulseOximeter.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "cJSON.h"
#include <Adafruit_TMP117.h>
#include <Adafruit_Sensor.h>

#define REPORTING_PERIOD_MS 1000
 
#define MQTT_SERVER   "hub.dev.selex.vn"
#define MQTT_PORT     1883
#define MQTT_USER     "selex"
#define MQTT_PASS     "selex"
#define MQTT_TOPIC    "mandevice/human/device_data/create"

/******************** WIFI Configuration ********************/
#define SSID          "Song Quynh"
#define WIFI_PASS     "songquynh25042112"
/******************** User Function Pre-Defination ********************/
void setup_wifi();
void connect_to_broker();

/******************** User Variable Defination ********************/
PulseOximeter pox;
uint32_t tsLastReport = 0;
// Adafruit_TMP117  tmp117;
WiFiClient            wifiClient;
PubSubClient          client(wifiClient);
float                 beatAvg;
int                   oxygen_percent = 96;
const char*           ssid = SSID;
const char*           password = WIFI_PASS;
char                  publish_data[100];

void onBeatDetected()
{
  Serial.println("Beat!");
}
  
void setup()
{
  Serial.begin(115200); 
  //Wifi setup
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT );
  connect_to_broker();

  Serial.print("Initializing pulse oximeter..");
  
  if (!pox.begin()) {
    Serial.println("FAILED");
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }

  // while (!tmp117.begin(0x10)) {
  //   Serial.println("Failed to find TMP117 chip");
  //   delay(1000);
  // }
  // Serial.println("TMP117 Found!");

}
 
void loop()
{ 
  client.loop();
  if (!client.connected()) {
    connect_to_broker();
  }
  pox.update();
  if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
    Serial.print("Heart BPM:");
    beatAvg = pox.getHeartRate();
    Serial.print(pox.getHeartRate());
    Serial.print("-----");
    Serial.print("Oxygen Percent:");
    Serial.print(pox.getSpO2());
    oxygen_percent = pox.getSpO2();
    Serial.println("\n");
    tsLastReport = millis();

    // sensors_event_t temp; // create an empty event to be filled
    // tmp117.getEvent(&temp); //fill the empty event object with the current measurements
    // Serial.print("Temperature  "); 
    // Serial.print(temp.temperature);Serial.println(" degrees C");
    // Serial.println("");

    memset(publish_data, 0, 100);
   
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "heart_beat_bpm", beatAvg);
    cJSON_AddNumberToObject(root, "oxygen_percent", oxygen_percent);

    cJSON_PrintPreallocated(root,publish_data,100, true);
    client.publish(MQTT_TOPIC, publish_data);
  }
}

void setup_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_to_broker() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 2 seconds");
      delay(2000);
    }
  }
}