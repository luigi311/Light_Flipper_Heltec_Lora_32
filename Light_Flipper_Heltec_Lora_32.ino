// Light switch flipper using a servo connected to a Heltec Wifi Lora 32 that utilizes an ESP32 chip.

//Imported headers
#include <WiFi.h>
#include <WiFiUdp.h>
#include <U8x8lib.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>

// Initialize the Servo
Servo servo;
int minUs = 500;                   // Minimum Pulse for TowerPro SG90 micro servos
int maxUs = 2400;                  // Maximum Pulse for TowerPro SG90 micro servos
//int minUs = 740;                     // Minimum Pulse for TowerPro MG92B micro servos
//int maxUs = 2308;                    // Maximum Pulse for TowerPro MG92B micro servos
int servoPin = 22;                   // Pin that connects to the PWM cable on servo
int offset = 15;                    // Offset for servo to account for small variations in 3d printing and mounting
int initialPos = 60 + offset;        // Base position in degrees
int offPos = 20 + offset;            // Off position in degrees
int onPos = 100 + offset;            // On position in degrees 
int movementSpeed = 1;               // Speed at which the servo moves at
int servoDelay = 15;                 // Delay to wait to give servo enough time to move to its new location

// OLED specific variables
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);

// Network Settings
const char* ssid     = "";
const char* password = "";

// MQTT Setup
const char* mqtt_server = "192.168.1.200";              // IP address of the MQTT broker
const char* boardName = "Lora";                         // Name of the board being used
const char* outTopic = "lightSwitchOut";                // Topic for the board to reply to when the task is complete
const char* inTopic = "/livingRoom/lightSwitchIn";      // Topic for the board to subscribe too in order to recieve commands

WiFiClient espClient;
PubSubClient client(espClient);

void setup(){
  Serial.begin(9600);
  
  servo.attach(servoPin, minUs, maxUs); // Attach the Servo to a pin
  servo.write(initialPos);              // Set the servo to its initial position
  
  // OLED screen initialization
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  
  // WIFI connect
  delay(10);
  wifi_connect();
  
  // MQTT setup
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

// Turn the lights on
void turnLightOn(){
  int pos;
  Serial.print("Turning Lights On");
  
  // Move servo to on postion
  for (pos = initialPos; pos < onPos; pos += movementSpeed) { 
    servo.write(pos);              
    delay(servoDelay);                          
  }

  // Move servo back to initial position
  for(pos = onPos; pos > initialPos; pos -= movementSpeed) {                                
    servo.write(pos);                   
    delay(servoDelay);                         
  } 
  Serial.println();
}

// Turn the lights off
void turnLightOff(){
  int pos;
  Serial.print("Turning Lights Off");
  
  // Move servo to off position
  for(pos = initialPos; pos > offPos; pos -= movementSpeed) {
    servo.write(pos);              
    delay(servoDelay);                        
  }

  // Move servo back to initial position
  for(pos = offPos; pos < initialPos; pos += movementSpeed) {                                  
    servo.write(pos);               
    delay(servoDelay);                       
  }
  Serial.println();
}

// Publish a message to the MQTT server
void pub(char *msg){
  Serial.print("Publish message: ");
  Serial.println(msg);
  client.publish(outTopic, msg);
}

// Listen for incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int len) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < len; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println(); 

  // Recieve the first character in the MQTT message which should be an integer
  int value = payload[0]-48;
  char *msg;
  
  if( len > 1 || value >= 2) {
    // If the MQTT message is longer than 1 character or is an integer larger than 1 return error in MQTT
    msg = "Invalid Option"; 
  } else {
    if(value == 1){
      turnLightOn();
      msg = "Turned light on";
    } else {
      turnLightOff();
      msg = "Turned light off";
    }
  }
  
  pub(msg);
}

// Connect to MQTT Server
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt connection
    if (client.connect(boardName)) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish(outTopic, "Connected");
      u8x8.print("\nMQTT Connected");
      // Resubscribe to inTopic
      client.subscribe(inTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}

// Connect to Wifi
void wifi_connect(){
  // SSID notification.
  Serial.print("Connecting to ");
  u8x8.print("Connecting to \n");
  Serial.println(ssid);
  u8x8.print(ssid);

  // Delete old config
  WiFi.disconnect(true);

  // Register event handler
  WiFi.onEvent(WiFiEvent);

  // Attempt WIFI connection
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    u8x8.print(".");
    delay(300);
  }
}

// Check Wifi connection state
void WiFiEvent(WiFiEvent_t event){
  switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
      Serial.print("WiFi connected! IP address: ");
      Serial.println(WiFi.localIP()); 
      u8x8.clear(); 
      u8x8.print("IP Address:\n");
      u8x8.print(WiFi.localIP());
      
      break;
      
    case SYSTEM_EVENT_STA_DISCONNECTED:
      Serial.println("WiFi lost connection");
      
      u8x8.print("\n");
      u8x8.print("WiFi lost connection");
      
      break;
  }
}


