/*
 Title: WSN Project - Base Station
 Written by: Carl Sagrado
 Student No: X00084403
 Date: 29/01/2021
 Current version: v1.3
**Version Updates:
 v1.0 - Receive data from Sensor 1 and Sensor 2
 v1.2 - Convert sensor data to a json format
 v1.3 - Detect and read data messages
**Desc:
 - The base station acts as a sink node of the WSN. This node receives all
 the sensor data and forward to the web server.
 - Base station transmits data received from server to designated sensor node
*/
/*Import libraries */
#include <esp_now.h> //include ESPOW library for ESP32 to this program 
#include <WiFi.h> //include Wi-Fi library to this program 
#include <TaskScheduler.h> //include task scheduler library to this program 
#include <ArduinoJson.h> //include json library for arduino 
Scheduler userScheduler; // to control task tas
Scheduler userScheduler1; // to control another task
//structure for serial data
typedef struct switchStat {
 int LED;
} switchStat;
//structure for wireless data
typedef struct struct_message {
 float temp; //temperature
 float hum; //humidity
 String device; // device name
 int LEDstate; //remote device state
} struct_message;
//Receiver MAC address
uint8_t receiver[] = {0x10, 0x52, 0x1C, 0x62, 0xB4, 0x08}; //Sensor Node 1
uint8_t receiver1[] = {0xF0, 0x08, 0xD1, 0xD2, 0x83, 0xF0}; //Sensor Node 2
// Define variables to store incoming readings
float incomingTemp;
float incomingHum;
String deviceName;
int ledStateR;
// Variable to store if sending data was successful
String success;
switchStat nodeRD1; //structure to send to node 1
switchStat nodeRD2; //structure to send to node 2
// Create a struct_message to hold incoming sensor readings
struct_message incomingReadings;
//Task scheduler function - for sending data to Sensor Node 1
Task taskSendMessage( TASK_SECOND * 0.5, TASK_FOREVER, []() {
 esp_err_t result = esp_now_send(receiver, (uint8_t *)&nodeRD1, sizeof(nodeRD1)); /
/ Send message via ESP-NOW
}); //Execute after 500ms per function call forever
Page 52 of 72
//Task scheduler function - for sending data to Sensor Node 2
Task taskSendMessage1( TASK_SECOND * 0.5, TASK_FOREVER, []() {
 esp_err_t result = esp_now_send(receiver1, (uint8_t *)&nodeRD2, sizeof(nodeRD2));
// Send message via ESP-NOW
});//Execute after 500ms per function call forever
// Callback when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
 memcpy(&incomingReadings, incomingData, sizeof(incomingReadings));
 incomingTemp = incomingReadings.temp; //readings for temperature
 incomingHum = incomingReadings.hum; //readings for humidity
 deviceName = incomingReadings.device; //device name
 ledStateR = incomingReadings.LEDstate; //remote device status
 receviedData(); //function call for
}
//function to convert received data to JSON format to send to server
void receviedData() {
 String msg2serial;
 DynamicJsonDocument message(1024);
 //Serial.printf("Received: LED:%s Temp: %sc Hum: %s\n", LED, Temp, Hum);
 message["LEDState"] = ledStateR;
 message["TEMP"] = incomingTemp ;
 message["HUM"] = incomingHum;
 message["deviceName"] = deviceName;
 serializeJson(message, msg2serial);
 Serial.println(msg2serial); //Send to Server via serial
}
void setup() {
 // Init Serial Monitor
 Serial.begin(115200);
 // Set device as a Wi-Fi Station
 WiFi.mode(WIFI_STA);
 // Init ESP-NOW
 if (esp_now_init() != ESP_OK) {
 Serial.println("Error initializing ESP-NOW");
 return;
 }
 // Once ESPNow is successfully Init, we will register for Send CB to
 // get the status of Trasnmitted packet
 esp_now_register_send_cb(OnDataSent);
 
// Register peer
 // Add peer
 esp_now_peer_info_t peerInfo;
 peerInfo.channel = 0;
 peerInfo.encrypt = false;
 memcpy(peerInfo.peer_addr, receiver, 6);
 if (esp_now_add_peer(&peerInfo) != ESP_OK) {
 Serial.println("Failed to add peer - MCU");
 return;
 }
 memcpy(peerInfo.peer_addr, receiver1, 6);
 if (esp_now_add_peer(&peerInfo) != ESP_OK) {
 Serial.println("Failed to add peer - External Node");
 return;
 }
Page 53 of 72
// Register for a callback function that will be called when data is received
 esp_now_register_recv_cb(OnDataRecv);
 userScheduler.addTask(taskSendMessage);
 taskSendMessage.enable();
 userScheduler1.addTask(taskSendMessage1);
 taskSendMessage1.enable();
}
/*End of setup() */
void loop() {
 if (Serial.available()) {
 String command = Serial.readStringUntil('\n');
 // Serial.println(command);
 if (command.equals("R1ON")) {
 nodeRD1.LED = 1;
 userScheduler.execute();
 // Serial.println(R1_LED.LED);
 } else if (command.equals("R1OFF")) {
 nodeRD1.LED = 0;
 userScheduler.execute();
 } else if (command.equals("R2ON")) {
 nodeRD2.LED = 1;
 //Serial.println(R1_LED.LED);
 userScheduler1.execute();
 // Serial.println(R1_LED.LED);
 } else if (command.equals("R2OFF")) {
 nodeRD2.LED = 0;
 userScheduler1.execute();
 } else if (command.equals("R2OFFR1OFF") || command.equals("R1OFFR2OFF")) {
 nodeRD1.LED = 0;
 nodeRD2.LED = 0;
 userScheduler.execute();
 userScheduler1.execute();
 } else if (command.equals("R2ONR1ON") || command.equals("R1ONR2ON")) {
 nodeRD1.LED = 1;
 nodeRD2.LED = 1;
 userScheduler.execute();
 userScheduler1.execute();
 } else { }
 }
}
/*End of void () */
