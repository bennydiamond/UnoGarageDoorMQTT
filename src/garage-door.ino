//
// Garage Door Activation with retries
// Required:
//
// Author:  psyko_chewbacca
// Date:    18-12-2018
// Version: 1.0
//
#include "GarageDoorState.h"
#include "StateChanger.h"
#include "WebServer.h"
#include "pins.h"
#include "strings.h"
// Define Libraries
#include <AM232X.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <limits.h>

// Define Sub's and Pub's
char const MQTTSubDoorSwitch[] PROGMEM = "garage/door/switch";
char const MQTTPubDoorStatus[] PROGMEM = "garage/door/status";
char const MQTTPubAvailable[] PROGMEM = "garage/door/available";
char const MQTTPubTemperature[] PROGMEM = "garage/door/temperature";
char const MQTTPubHumidity[] PROGMEM = "garage/door/humidity";
char const MQTTBrokerrUser[] PROGMEM = "sonoff";
#define MQTTBrokerrPass MQTTBrokerrUser
char const MQTTAvailablePayload[] PROGMEM = "online";
char const MQTTUnavailablePayload[] PROGMEM = "offline";

uint16_t const StateCheckInterval_ms PROGMEM = 1000;
uint16_t const PublishStatusInterval_ms PROGMEM = 2000;
uint16_t const PublishAvailableInterval_ms PROGMEM = 30000;
uint16_t const PublishTemperatureInterval_ms PROGMEM = 60000;
uint8_t const willQos PROGMEM = 0;
uint8_t const willRetain PROGMEM = 0;

// MQTT Broker
IPAddress const MQTT_SERVER(192, 168, 1, 254) PROGMEM;

// The IP address of the Arduino
uint8_t const mac[] PROGMEM = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

// Remove remarks and alter if you want a fixed IP for the Arduino
IPAddress const ip(192, 168, 1, 200) PROGMEM;
IPAddress const dns(192, 168, 1, 254) PROGMEM;
IPAddress const gateway(192, 168, 0, 1) PROGMEM;
IPAddress const subnet(255,255,254,0) PROGMEM;


// Delay timer
static uint32_t publishStatusTimer;
static uint32_t publishAvailabilityTimer;
static uint32_t publishTemperatureTimer;
static uint32_t StateCheck = 0;
static unsigned long previous;

// Ethernet Initialisation
EthernetClient ethClient;
GarageDoorState garageDoorState;
StateChanger stateChanger;
WebServer webServer;

AM232X AM2320;

// Callback to Arduino from MQTT (inbound message arrives for a subscription - in this case the door on/off switch)
void callback (char* topic, uint8_t* payload, unsigned int length) 
{
  // MQTT Inbound messaging 
  char message_buff[MQTT_MAX_PACKET_SIZE];
  
  memcpy(message_buff, payload, length);
  message_buff[length] = '\0';

  // Convert buffer to string
  webServer.placeString("Inbound: ");
  webServer.placeString(topic);
  webServer.placeString(":");
  webServer.placeString(message_buff);
  webServer.placeString(LINE_BREAK);
  
  // Only open if not already open, only close if not already closed
  if(0 == strcmp(message_buff,"Open")) 
  {
    stateChanger.stateChangeRequest(DoorState_Open);
  }
  else if(0 == strcmp(message_buff, "Close"))
  {
    stateChanger.stateChangeRequest(DoorState_Close);
  }
  else if(0 == strcmp(message_buff, "Toggle"))
  {
    stateChanger.toggleRelay();
  }
}

// Define Publish / Subscribe client (must be defined after callback function above)
PubSubClient mqttClient(MQTT_SERVER, 1883, callback, ethClient);


void setup (void) 
{
    // Start Serial
  Serial.begin(115200);
  webServer.placeString("Hello World!" LINE_BREAK);

  uint32_t shift = 0;

  publishStatusTimer = shift;
  shift += 17;
  publishAvailabilityTimer = shift;
  shift += 17;
  publishTemperatureTimer = shift;

  stateChanger.bindGarageDoorState(&garageDoorState);
  stateChanger.bindWebServer(&webServer);
  garageDoorState.bindWebServer(&webServer);

  // LED off, visual indicator when triggered
  webServer.placeString(String_HelloWorld, false);

  previous = 0;

  // Start Network (replace with 'Ethernet.begin(mac, ip);' for fixed IP)
  Ethernet.begin(const_cast<uint8_t *>(mac), ip, dns, gateway, subnet);
  webServer.placeString("Ethernet init" LINE_BREAK);

  // Digital IO 17 act as GND for AM2320B sensor
  pinMode(AM2320_PINGND, OUTPUT);
  digitalWrite(AM2320_PINGND, LOW);
  // Digital IO 16 act as +5V for AM2320B sensor
  pinMode(AM2320_PINVCC, OUTPUT);
  digitalWrite(AM2320_PINVCC, HIGH);

  AM2320.begin();
  webServer.placeString("Temp/Hum sensor init" LINE_BREAK);

  // Let network have a chance to start up
  delay(1500);

  // Display IP for debugging purposes
  printIPAddress();

}

void loop (void) 
{
  // If not MQTT connected, try connecting
  if(!mqttClient.connected())  
  {
    // Connect to MQTT broker on the openhab server, retry constantly
    while(mqttClient.connect(String_PorteDeGarage, MQTTBrokerrUser, MQTTBrokerrPass, MQTTPubAvailable, willQos, willRetain, MQTTUnavailablePayload) != 1) 
    {
      char szErrorCode[10];
      webServer.placeString(String_ErrorConnectMQTTState);
      snprintf(szErrorCode, 10, "%d", (char)mqttClient.state());
      webServer.placeString(szErrorCode);
      webServer.placeString(")" LINE_BREAK);
      delay(1000);
    }
    
    // Subscribe to the activate switch on OpenHAB
    mqttClient.subscribe(MQTTSubDoorSwitch);
  }
  
  // Wait a little bit between status checks...
  if(0 == publishStatusTimer) 
  {
    // Where are we right now
    publishStatusTimer = PublishStatusInterval_ms;

    // Publish Door Status
    // Send message
    PublishMQTTMessage(MQTTPubDoorStatus, garageDoorState.getDoorString());  
    webServer.placeString(String_PubDoorState__);
    webServer.placeString(garageDoorState.getDoorString());
    webServer.placeString(LINE_BREAK);
  }
  if(0 == publishAvailabilityTimer)
  {
    publishAvailabilityTimer = PublishAvailableInterval_ms;

    PublishMQTTMessage(MQTTPubAvailable, MQTTAvailablePayload);  
    webServer.placeString(String_Pub_);
    webServer.placeString(MQTTAvailablePayload);
    webServer.placeString(LINE_BREAK);
  }
  if(0 == publishTemperatureTimer)
  {
    publishTemperatureTimer = PublishTemperatureInterval_ms;

    if(AM232X_OK == AM2320.read())
    {
      float const temp = AM2320.temperature;
      float const hum = AM2320.humidity;

      char format[7];

      dtostrf(temp, 4, 2, format);
      PublishMQTTMessage(MQTTPubTemperature, format);  
      webServer.placeString(String_Pub_);
      webServer.placeString(format);
      webServer.placeString(LINE_BREAK);


      dtostrf(hum, 4, 2, format);
      PublishMQTTMessage(MQTTPubHumidity, format);  
      webServer.placeString(String_Pub_);
      webServer.placeString(format);
      webServer.placeString(LINE_BREAK);
    }
  }
  // Do it all over again
  mqttClient.loop();

  advanceTimers();

  if(0 == StateCheck)
  {
    garageDoorState.run();
    StateCheck = StateCheckInterval_ms;
  }

  stateChanger.run();

  webServer.run();
}


// Print IP for debugging, can be removed if needed
void printIPAddress (void)
{
  Serial.print("My IP address: ");
  
  for(uint8_t thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }

  Serial.print(LINE_BREAK);
}


// Publish MQTT data to MQTT broker
void PublishMQTTMessage (char const * const sMQTTSubscription, char const * const sMQTTData)
{
  // Define and send message about door state
  mqttClient.publish(sMQTTSubscription, sMQTTData); 

  // Debug info
  webServer.placeString("Outbound: ");
  webServer.placeString(sMQTTSubscription);
  webServer.placeString(":");
  webServer.placeString(sMQTTData);
  webServer.placeString(LINE_BREAK);
 
}

static void advanceTimers (void)
{
  unsigned long const current = millis();
  if(current != previous)
  {
    previous = current;

    if(StateCheck)
    {
      StateCheck--;
    }

    if(publishStatusTimer)
    {
      publishStatusTimer--;
    }

    if(publishAvailabilityTimer)
    {
      publishAvailabilityTimer--;
    }

    if(publishTemperatureTimer)
    {
      publishTemperatureTimer--;
    }
  }
}
