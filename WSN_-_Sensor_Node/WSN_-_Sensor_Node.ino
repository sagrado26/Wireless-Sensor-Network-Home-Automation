/*
 Title: WSN Project - Sensor Node
 Written by: Carl Sagrado
 Student No: X00084403
 Date: 29/01/2021
 Current version: v1.6
**Version Updates:
 v1.0 - Detect Temp and Humidity
 v1.1 - Added ESP_NOW for sending and receiving data wirelessly
 v1.2 - Added Task scheduler as an alternative solution .
 to remove unnecessary use of delay into the function
 v1.3 - Modified to execute received intructions and to
 automatically send status to after execution
 v1.4 - Modified to communicate to the base station
 v1.5 - Modified to communicate to the end device
 v1.6 - Modified to forward rel information between base station and end device
**Desc:
 - Communicates to the base station
 - Communicates to the remote device
*/
#include <esp_now.h> //include ESPOW library for ESP32 to this program 
#include <WiFi.h> //include Wi-Fi library to this program 
#include <TaskScheduler.h> //include task scheduler library to this program 
#include <ArduinoJson.h> //include json library for arduino 
#include "DHT.h" //include DHT sensor library to this program
#define DHTPIN 22 //DHT Pin
#define DHTTYPE DHT11 // DHT 11 
DHT dht(DHTPIN, DHTTYPE); //create dht object
Scheduler userScheduler; // to control your personal task
Scheduler userScheduler1; // to control your personal task
// MAC Address of receiver
uint8_t receiver[] = {0x10, 0x52, 0x1C, 0x5D, 0x62, 0x50}; //MASTER
uint8_t receiver1[] = {0xF0, 0x08, 0xD1, 0xD3, 0x3D, 0xF0}; //EXTERNAL NODE
//data Structure for msg received and sent by the base station
typedef struct struct_message {
 float temp;
 float hum;
 String device;
 int LEDstate;
} struct_message;
//data Structure for msg received and sent by end devicen
typedef struct switchStat {
 int LED;
} switchStat;
// variables to store sensor readings
float t;
float h;
String deviceN = "Room 1"; //Location of the device
Page 48 of 72
// Variable to store if sending data was successful
String success;
switchStat newStatus;
switchStat currentStatus;
switchStat outgoing;
struct_message readings; //Structure example to send data (must match with the recei
veer
struct_message incomingReadings; // Create a struct_message to hold incoming sensor
readings
// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
 Serial.print("\r\nLast Packet Send Status:\t");
 Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fai
l");
 if (status == 0) {
 success = "Delivery Success :)";
 }
 else {
 success = "Delivery Fail :(";
 }
}
/****** RECEIVING DATA (function) *********/
// Callback when data is received
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
 char macStr[18];
 String x;
//Store mac address to an Array
 snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
 mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr
[5]);
//add all MAC values and convert to a decimal
 x = mac_addr[0] + mac_addr[1] + mac_addr[2] + mac_addr[3] + mac_addr[4] + mac_addr
[5];
 
 if (x == "397") { //Detect msgs transmitted by the base station
 memcpy(&newStatus, incomingData, sizeof(newStatus)); //copy to memory
 newState = newStatus.LED; //extract rcvd data by assigning to a global variable
 Serial.println("FROM MCU -" );
 displayReceviedData();
 Serial.print("LED newState: ");
 Serial.print(newState);
 Serial.println();
 userScheduler1.execute(); //call task function to forward data to end device
 }
if (x == "969") { //F0:08:D1:D3:3D:F0 - Detect msgs sent by end device
 Serial.print("EXTERNAL NODE - ");
 memcpy(&currentStatus, incomingData, sizeof(currentStatus)); //copy to memory
 state = currentStatus.LED; //extract rcvd data by assigning to a global var
 Serial.print("LED State: ");
 Serial.print(state);
 Serial.println();
 }
}
/****** Display received DATA (function) *********/
void displayReceviedData() { //function to display the msg received from the base st
ation
 Serial.printf("New LED status: %d \n", newState);
}
Page 49 of 72
//Task scheduler function - for sending data to the base station
Task taskSendMessage( TASK_SECOND * 1, TASK_FOREVER, []() {
//Execute after 1000s per function call forever
 readings.temp = t;
 readings.hum = h;
 readings.device = deviceN;
 readings.LEDstate = state;
 esp_err_t result = esp_now_send(receiver, (uint8_t *)&readings, sizeof(readings));
// Send message via ESP-NOW
 if (result == ESP_OK) { //check transmission status
 Serial.println("Sent with success");
 }
 else {
 Serial.println("Error sending the data");
 }
}); // start with a one second interval
//Task scheduler function - for sending data to the remove device
Task taskSendMessage1( TASK_SECOND * 0.5, TASK_FOREVER, []() {
 outgoing.LED = newState ;
 Serial.println(outgoing.LED);
 esp_err_t result = esp_now_send(receiver1, (uint8_t *)&outgoing, sizeof(outgoing))
; // Send message via ESP-NOW
 if (result == ESP_OK) { //check transmission status
 Serial.println("Sent to an end device with success");
 }
 else {
 Serial.println("Error sending the data");
 }
}); // start with a one second interval
void setup() {
 // Init Serial Monitor
 Serial.begin(115200);
 Serial.println(F("DHTxx test!"));
 dht.begin();
 
 // Set device as a Wi-Fi Station
 WiFi.mode(WIFI_STA);
 if (esp_now_init() != ESP_OK) { // Initialize ESP-NOW
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
Page 50 of 72
memcpy(peerInfo.peer_addr, receiver1, 6);
 if (esp_now_add_peer(&peerInfo) != ESP_OK) {
 Serial.println("Failed to add peer - External Node");
 return;
 }
 // Register for a callback function that will be called when data is received
 esp_now_register_recv_cb(OnDataRecv);
 userScheduler.addTask(taskSendMessage);
 userScheduler1.addTask(taskSendMessage1);
 taskSendMessage.enable();
 taskSendMessage1.enable();
}
void loop() {
 t = dht.readTemperature(); //read temperature
 h = dht.readHumidity(); //read humidity
 userScheduler.execute(); //call task scheduler to send update
}
