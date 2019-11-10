#include <M5StickC.h>
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include "MTCH6102.h"

#define ADDR 0x25
MTCH6102 mtch = MTCH6102();
int len = 8;

char *ssid = "";
char *password = "";
const char *endpoint = "";
const int port = 1883;
char *deviceID = "M5Stack";
char *subTopic = "nuno/midi/out/+/+/+";
char *pubTopic[] = {
  "nuno/midi/out/1/noteon/60", // C
  "nuno/midi/out/1/noteon/59", // B
  "nuno/midi/out/1/noteon/57", // A
  "nuno/midi/out/1/noteon/55", // G
  "nuno/midi/out/1/noteon/53", // F
  "nuno/midi/out/1/noteon/52", // E
  "nuno/midi/out/1/noteon/50", // D
  "nuno/midi/out/1/noteon/48"  // C
};

WiFiClient httpsClient;
PubSubClient mqttClient(httpsClient);

void setup() {
  byte data;
  Serial.begin(115200);

  // Initialize the M5Stack object
  M5.begin();
  M5.Lcd.setRotation(1); // display size = 80x160, setRotation = 0:M5, 1:Power Btn, 2:up side down, 3:Btn
  M5.Lcd.fillScreen(TFT_BLACK);

  // WiFi Start
  Serial.println("Connecting to ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);
     while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");

  // MQTT Start
  mqttClient.setServer(endpoint, port);
  connectMQTT();
   
  //Wire.begin();
  mtch.begin();
  mtch.writeRegister(MTCH6102_MODE, MTCH6102_MODE_STANDBY);
  mtch.writeRegister(MTCH6102_NUMBEROFXCHANNELS, 0x10 );
  mtch.writeRegister(MTCH6102_NUMBEROFYCHANNELS, 0x03); //最低3点必要なため
  mtch.writeRegister(MTCH6102_MODE, MTCH6102_MODE_FULL);
  
  mtch.writeRegister(MTCH6102_CMD, 0x20);
  delay(500);
  
  // the operating mode (MODE)
  data = mtch.readRegister(MTCH6102_MODE);
  Serial.print("MODE: ");
  Serial.println(data,BIN);
  data = mtch.readRegister(MTCH6102_NUMBEROFXCHANNELS);
  Serial.print("NUMBER_OF_X_CHANNELS: ");
  Serial.println(data);
  data = mtch.readRegister(MTCH6102_NUMBEROFYCHANNELS);
  Serial.print("NUMBER_OF_Y_CHANNELS: ");
  Serial.println(data);
  
}

void connectMQTT() {
    while (!mqttClient.connected()) {
        if (mqttClient.connect(deviceID)) {
            Serial.println("MQTT Connected.");
            int qos = 0;
            mqttClient.subscribe(subTopic, qos);
        } else {
            Serial.print("Failed. Error state=");
            Serial.println(mqttClient.state());
            delay(1000);
        }
    }
}

void mqttLoop() {
    if (!mqttClient.connected()) {
        connectMQTT();
    }
    mqttClient.loop();
}

int sensStats[10] = {0,0,0,0,0,0,0,0,0};
float sensVals[10] = {0,0,0,0,0,0,0,0,0,0};

void loop() {
  byte data; 
  mqttLoop();
  
  // M5.lcd.clear();
  M5.Lcd.setCursor(5, 5); 
  M5.Lcd.print("                                                ");
  M5.Lcd.setCursor(5, 5);

  //Serial.print("SENSORVALUE_RX <i>: ");
  //for (byte i = MTCH6102_SENSORVALUE_RX0; i < MTCH6102_SENSORVALUE_RX0+10; i++) {
  for (int i = 0; i < len; i++) {
    data = mtch.readRegister(MTCH6102_SENSORVALUE_RX0+i);
    sensVals[i] = data;

    // Serial.print(data);
    // Serial.print(", ");
    M5.Lcd.print(data);
    M5.Lcd.print(", ");

    // Low Water Mark
    if (sensVals[i] < 150) {
      if (sensStats[i] > 0) {
        mqttClient.publish(pubTopic[i], "0");
        Serial.print(i);
        Serial.println(": off Published.");
        sensStats[i] = 0;
      }
    }

    // High Water Mark
    if (sensVals[i] > 200) {
      if (sensStats[i] < 1) {
        mqttClient.publish(pubTopic[i], "100");
        Serial.print(i);
        Serial.println("on Published.");
        sensStats[i] = 1;
      }
    }
  }

  // Serial.println();

  for (int i = 0; i<len; i++){
    float prev;
    if(i==0){
       prev = 0;
    }else{
       prev=sensVals[i-1];
    }
    M5.Lcd.drawLine(i*20, 80-(prev/255)*80, (i+1)*20, 80-(sensVals[i]/255)*80, TFT_WHITE);
  }

  delay(50);
  
  for (int i = 0; i<len; i++){
    float prev;
    if(i==0){
       prev = 0;
    }else{
       prev=sensVals[i-1];
    }
    M5.Lcd.drawLine(i*20, 80-(prev/255)*80, (i+1)*20, 80-(sensVals[i]/255)*80, TFT_BLACK);
  }

  M5.Lcd.println();
  
  //delay(100);
 }
  
