#include <Arduino.h>
#include "WiFi.h"
#include "PubSubClient.h"
#include <Wire.h>
#include "MAX30105.h"           //MAX3010x library
#include "heartRate.h"          //Heart rate  calculating algorithm
#include "Adafruit_TMP117.h"
#include "Adafruit_Sensor.h"
#include "cJSON.h"

/******************** MQTT Defination ********************/
#define MQTT_SERVER   "YOUR_MQTT_SERVER"
#define MQTT_PORT     1883
#define MQTT_USER     "USER_NAME"
#define MQTT_PASS     "PASSWORD"
#define MQTT_TOPIC    "mandevice/human/device_data/create"

/******************** WIFI Configuration ********************/
#define SSID          "Wifi name"
#define WIFI_PASS     "Wifi pass"

/******************** User Function Pre-Defination ********************/
void setup_wifi();
void connect_to_broker();

/******************** User Variable Defination ********************/
MAX30105              particleSensor;
Adafruit_TMP117       tmp117;
WiFiClient            wifiClient;
PubSubClient          client(wifiClient);

const byte            RATE_SIZE  = 4; //Increase this for more averaging. 4 is good.
byte                  rates[RATE_SIZE]; //Array  of heart rates
byte                  rateSpot = 0;
long                  lastBeat = 0; //Time at which the last  beat occurred
float                 beatsPerMinute;
int                   beatAvg;
int                   oxygen_percent = 90;
const char*           ssid = SSID;
const char*           password = WIFI_PASS;
char                  publish_data[100];
/******************** Setup Begin ********************/
void setup() {  
  Serial.begin(115200); 
  //Wifi setup
  setup_wifi();
  client.setServer(MQTT_SERVER, MQTT_PORT );
  connect_to_broker();
  //  Initialize sensor
   if (!tmp117.begin()) {
    Serial.println("Failed to find TMP117 chip");
    while (1) { delay(10); }
  }
  Serial.println("TMP117 Found!");
  particleSensor.begin(Wire, I2C_SPEED_FAST); //Use default  I2C port, 400kHz speed
  particleSensor.setup(); //Configure sensor with default  settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to  indicate sensor is running
}


/******************** Infinite Loop ********************/
void loop() {

  client.loop();
  if (!client.connected()) {
    connect_to_broker();
  }

  long irValue = particleSensor.getIR();    //Reading the IR value it will permit us to know if there's a finger on the  sensor or not
  if(irValue > 7000){                                           //If a finger is detected

    if (checkForBeat(irValue) == true)                        //If  a heart beat is detected
    {
      tone(3,1000);                                        //And  tone the buzzer for a 100ms you can reduce it it will be better
      noTone(3);                                          //Deactivate the buzzer  to have the effect of a "bip"
      //We sensed a beat!
      long delta = millis()  - lastBeat;                   //Measure duration between two beats
      lastBeat  = millis();
      beatsPerMinute = 60 / (delta / 1000.0);           //Calculating  the BPM
      
      if (beatsPerMinute < 255 && beatsPerMinute > 20)               //To  calculate the average we strore some values (4) then do some math to calculate the  average
      {
        rates[rateSpot++] = (byte)beatsPerMinute; //Store this  reading in the array
        rateSpot %= RATE_SIZE; //Wrap variable
        //Take  average of readings
        beatAvg = 0;
        for (byte x = 0 ; x < RATE_SIZE  ; x++)
          beatAvg += rates[x];
        beatAvg /= RATE_SIZE;
      }
    }
  }
  if (irValue < 7000){       //If no finger is detected it inform  the user and put the average BPM to 0 or it will be stored for the next measure
     beatAvg=0;
     noTone(3);
  }
  sensors_event_t temp; // create an empty event to be filled
  tmp117.getEvent(&temp); //fill the empty event object with the current measurements
  Serial.print("Temperature  "); 
  Serial.print(temp.temperature);
  Serial.println(" degrees C");

  memset(publish_data, 0, 100);
  cJSON* root = cJSON_CreateObject();
  cJSON_AddNumberToObject(root, "heart_beat_bpm", beatAvg);
  cJSON_AddNumberToObject(root, "oxygen_percent", oxygen_percent);
  cJSON_AddNumberToObject(root, "temperature", temp.temperature);
  cJSON_PrintPreallocated(root,publish_data,100, true);
  client.publish(MQTT_TOPIC, publish_data);
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