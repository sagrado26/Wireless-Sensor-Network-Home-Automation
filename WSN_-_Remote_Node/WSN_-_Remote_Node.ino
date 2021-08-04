/* Title: WSN Project - Remote Device
 Written by: Carl Sagrado
 Student No: X00084403
 Date: 29/01/2021
 Current version: v1.3
**Version Updates:
 v1.0 - Switch button added to the circuit to control relay.
 v1.1 - Added ESP_NOW for sending and receiving data wirelessly.
 v1.2 - Added Task scheduler as an alternative solution .
 to remove unnecessary use of delay into the function
 v1.3 - Modified to execute received instructions and to
 automatically send status to after execution
**Desc:
 - This is an end device controller that contols the status a relay switch
*/
#include <esp_now.h> //include ESPOW library for ESP32 to this program 
#include <WiFi.h> //include Wi-Fi library to this program 
#include <TaskScheduler.h> //include task scheduler library to this program 
#define Relay 22 //Relay pinout
#define switch 21 //Physical switch pinout
//Receiver MAC Address
uint8_t receiver[] = {0x10, 0x52, 0x1C, 0x62, 0xB4, 0x08}; //Sensor Node 1
//data Structure format for switch data
typedef struct LEDstate {
 int LEDstatus; //LED status
} LEDstate;
LEDstate newLedStatus; //Structure for received switch instruction.
LEDstate currentStatus; //Structure used to send current switch status.
/*Global variables */
int LEDState; //switch status
int LEDnewState; //varaible
int buttonNew; //used to read physical button status
int buttonOld = 1; //previous button status
Scheduler userScheduler; // to control sending of msg task
Scheduler executeMsg // to control execution of msg task
/*---------Call back function to check status of sent data -------------*/
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
 Serial.print("\r\nLast Packet Send Status:\t");
 //Check transmission status and display to screen [Success or Fail]
 Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fai
l");
 if (status == 0) {
 success = "Delivery Success :)";
 }
 else {
 success = "Delivery Fail :(";
 }
}
Page 45 of 72
/*--------- function for receiving data -------------*/
void OnDataRecv(const uint8_t * mac_addr, const uint8_t *incomingData, int len) {
 memcpy(&newLedStatus, incomingData, sizeof(newLedStatus)); //store data to memory
 LEDState = newLedStatus.LEDstatus; //assign received data to variable
 Serial.print("I AM RECEIVING THE DATA"); //Display received data to serial monitor
 Serial.println(LEDState);
 //When data is recevied, schedule to execute msg received
 executeMsg.execute();
}
/Task scheduler function - for executing the received msg
Task executeMessage( TASK_SECOND * 0.5, TASK_FOREVER, []() {
//Execute after 500ms per function call forever
 if (LEDState == 0 ) { //Checks remote signal sent TURN OFF
 digitalWrite(Relay, LOW); //set pin to low
 LEDnewState = 0; //update status
 userScheduler.execute(); //send back the update
 } else if (LEDState == 1) { //TURN ON
 digitalWrite(Relay, HIGH); //set pin to high
 LEDnewState = 1; //update status
 userScheduler.execute(); //send the update
 } else { //do nothing }
});
//Task scheduler function - send update to sensor node
Task taskSendMessage( TASK_SECOND * 0.5, TASK_FOREVER, []() {
 //Execute after 500ms per function call forever
 currentStatus.LEDstatus = LEDnewState; //update the currentStatus structure value
 /*using esp_now_send function to send the data - MAC Address, the structure mess
age variable , and the size of the msg*/
 esp_err_t result = esp_now_send(receiver, (uint8_t *)&currentStatus, sizeof(curren
tStatus)); // Send message via ESP-NOW
 //Execute call back function to check if the receiver is online
 if (result == ESP_OK) {
 Serial.println("Sent with success"); //data sent successfully
 }
 else {
 Serial.println("Error sending the data"); //sending data failed 
 }
});
void setup() {
 // Initialize Serial Monitor
 Serial.begin(115200);
 pinMode(Relay, OUTPUT); //set relay pin to as an output pin
 pinMode(switch, INPUT); //set switch pin as an input pin
 // Set device as a Wi-Fi Station
 WiFi.mode(WIFI_STA);
 // Initialize ESP-NOW
 if (esp_now_init() != ESP_OK) {
 Serial.println("Error initializing ESP-NOW");
 return;
 }
Page 46 of 72
 /*---- Continuation of void setup() function ----- */
// Once ESPNow is successfully Init, we will register for Send CB to
 // get the status of Trasnmitted packet
 esp_now_register_send_cb(OnDataSent);
 // Register peer
 esp_now_peer_info_t peerInfo;
 memcpy(peerInfo.peer_addr, receiver, 6);
 peerInfo.channel = 0;
 peerInfo.encrypt = false;
 // Add peer
 if (esp_now_add_peer(&peerInfo) != ESP_OK) {
 Serial.println("Failed to add peer");
 return;
 }
 // Register for a callback function that will be called when data is received
 esp_now_register_recv_cb(OnDataRecv);
 userScheduler.addTask(taskSendMessage); //initialize scheduler for sending data
 taskSendMessage.enable(); //enable scheduler
 executeMsg.addTask(executeMessage); //initialize scheduler for executing rcvd msg
 executeMessage.enable(); //enable schedulure
} // end of setup()
void loop() {
 buttonNew = digitalRead(switch); //read switch status
 if ((buttonOld == 0 && buttonNew == 1)) { //Manual checking for manual switching
 if (LEDState == 0) { //check condition of LED
 digitalWrite(Relay, HIGH); //turn on LED
 LEDState = 1; //update led status
 delay(1000); //delay
 } else {
 digitalWrite(Relay, LOW); //turn off LED
 LEDState = 0; //update LED status
 delay(1000); //delay
 }
 userScheduler.execute(); //send new update
 }
 buttonOld = buttonNew;
} // end of void function
