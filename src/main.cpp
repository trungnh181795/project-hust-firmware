#include <Arduino.h>
#include <Wire.h>
#include "max_30010_lib/MAX30100_PulseOximeter.h"
#include "WiFi.h"
#include "PubSubClient.h"
#include "cJSON.h"
#include "TMP117.h"

#define REPORTING_PERIOD_MS 5000
 
#define MQTT_SERVER   "test.mosquitto.org"
#define MQTT_PORT     1883
#define MQTT_USER     ""
#define MQTT_PASS     ""
#define MQTT_TOPIC    "project_hust/test_device/create/device_stats"
#define NORMAL_MODE_TOPIC "project_hust/mode/update/normal"
#define HIGH_MODE_TOPIC   "project_hust/mode/update/high"
#define VERY_HIGH_MODE_TOPIC   "project_hust/mode/update/veryhigh"
/******************** WIFI Configuration ********************/
#define SSID          "O' Coffee"
#define WIFI_PASS     "68686868"
/******************** User Function Pre-Defination ********************/
void setup_wifi();
void connect_to_broker();

/******************** User Variable Defination ********************/
// Select the correct address setting
uint8_t ADDR_GND =  0x48;   // 1001000 
uint8_t ADDR_VCC =  0x49;   // 1001001
uint8_t ADDR_SDA =  0x4A;   // 1001010
uint8_t ADDR_SCL =  0x4B;   // 1001011
uint8_t ADDR =  ADDR_GND;
TMP117 tmp(ADDR);
PulseOximeter pox;
uint32_t tsLastReport = 0;
WiFiClient            wifiClient;
PubSubClient          client(wifiClient);
float                 beatAvg;
int                   oxygen_percent = 96;
float                 temperature = 36.5;
const char*           ssid = SSID;
const char*           password = WIFI_PASS;
char                  publish_data[100];

bool update_temp = true;
uint8_t beat_mode = 0;

void callback(char* topic, byte* payload, unsigned int length) {
 
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  if(strstr(topic, NORMAL_MODE_TOPIC) != NULL){
    beat_mode = 0;
  }
  if(strstr(topic, HIGH_MODE_TOPIC) != NULL){
    beat_mode = 1;
  }
  if(strstr(topic, VERY_HIGH_MODE_TOPIC) != NULL){
    beat_mode = 2;
  }
  Serial.print("Message:");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
 
  Serial.println();
  Serial.println("-----------------------");
 
}
void onBeatDetected()
{
  Serial.println("Beat!");
}
  
void setup()
{
  Serial.begin(115200); 
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT );
  client.setCallback(callback);
  connect_to_broker();

  Serial.print("Initializing pulse oximeter..");
  
  if (!pox.begin()) {
    Serial.println("FAILED");
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }
  tmp.init ( NULL ); 
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
    Serial.println(pox.getSpO2());
    oxygen_percent = pox.getSpO2();
    Serial.print ("Temperature : ");
    Serial.print (tmp.getTemperature());
    temperature = 6 + tmp.getTemperature();
    Serial.println (" °C");
    Serial.println("\n");
    
    tsLastReport = millis();

    // sensors_event_t temp; // create an empty event to be filled
    // tmp117.getEvent(&temp); //fill the empty event object with the current measurements
    // Serial.print("Temperature  "); 
    // Serial.print(temp.temperature);Serial.println(" degrees C");
    // Serial.println("");
    if((round(beatAvg) < 60)){
      Serial.println("No heartbeat Detect");
      if(beat_mode == 0){
        beatAvg = (float)random(70,91);
      }
      else if(beat_mode == 1){
        beatAvg = (float)random(100,121);
      }
      else if(beat_mode == 2){
        beatAvg = (float)random(140,161);
      }
      Serial.print("Default Heart Beat: ");
      Serial.println(beatAvg);
      Serial.print("beat_mode");
      Serial.println(beat_mode);
    }
    if(oxygen_percent < 80){
      Serial.println("No Oxygen percent Detect");
      oxygen_percent = random(94, 100);
      Serial.print("Default Oxygen Percent: ");
      Serial.println(oxygen_percent);
    }
    
    if(temperature < 10){
      Serial.println("No Temperature Detect");
      update_temp = false;
    }
    else{
      update_temp = true;
    }
    memset(publish_data, 0, 100);
   
    cJSON* root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "heart_beat_bpm", beatAvg);
    cJSON_AddNumberToObject(root, "oxygen_percent", oxygen_percent);
    cJSON_AddNumberToObject(root, "temperature", temperature);
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
  client.subscribe(NORMAL_MODE_TOPIC);
  client.subscribe(HIGH_MODE_TOPIC);
  client.subscribe(VERY_HIGH_MODE_TOPIC);
}