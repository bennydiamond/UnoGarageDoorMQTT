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
// Define Libraries
#include <AM232X.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <limits.h>

// Define Sub's and Pub's
#define MQTTSubDoorSwitch "garage/door/switch"
#define MQTTPubDoorStatus "garage/door/status"
#define MQTTPubAvailable "garage/door/available"
#define MQTTPubTemperature "garage/door/temperature"
#define MQTTPubHumidity "garage/door/humidity"
#define MQTTBrokerrUser "sonoff"
#define MQTTBrokerrPass "sonoff"
#define MQTTAvailablePayload "online"
#define MQTTUnavailablePayload "offline"

#define StateCheckInterval_ms 1000
#define PublishStatusInterval_ms 2000
#define PublishAvailableInterval_ms 30000
#define PublishTemperatureInterval_ms 60000
#define willQos (0)
#define willRetain (0)

// MQTT Broker
IPAddress MQTT_SERVER(192, 168, 1, 254);

// The IP address of the Arduino
uint8_t mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

// Remove remarks and alter if you want a fixed IP for the Arduino
IPAddress ip(192, 168, 1, 200);
IPAddress dns(192, 168, 1, 254);
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255,255,254,0);


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
  webServer.placeString("Hello World!" LINE_BREAK);

  previous = 0;

  // Start Network (replace with 'Ethernet.begin(mac, ip);' for fixed IP)
  Ethernet.begin(mac, ip, dns, gateway, subnet);

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
    while(mqttClient.connect("Porte de garage", MQTTBrokerrUser, MQTTBrokerrPass, MQTTPubAvailable, willQos, willRetain, MQTTUnavailablePayload) != 1) 
    {
      char szErrorCode[10];
      webServer.placeString("Error connecting to MQTT (State:");
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
    webServer.placeString("Pub Door state: ");
    webServer.placeString(garageDoorState.getDoorString());
    webServer.placeString(LINE_BREAK);
  }
  if(0 == publishAvailabilityTimer)
  {
    publishAvailabilityTimer = PublishAvailableInterval_ms;

    PublishMQTTMessage(MQTTPubAvailable, MQTTAvailablePayload);  
    webServer.placeString("Pub: ");
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
      snprintf(format, 7, "%.2f", static_cast<double>(temp));

      PublishMQTTMessage(MQTTPubTemperature, format);  
      webServer.placeString("Pub: ");
      webServer.placeString(format);
      webServer.placeString(LINE_BREAK);

      snprintf(format, 7, "%.2f", static_cast<double>(hum));

      PublishMQTTMessage(MQTTPubHumidity, format);  
      webServer.placeString("Pub: ");
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
