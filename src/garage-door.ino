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
#include <avr/wdt.h>

uint16_t const StateCheckInterval_ms = 1000;
uint16_t const PublishAvailableInterval_ms = 30000;
uint16_t const QueryClimateInterval_ms = 20000;
uint8_t const willQos = 0;
uint8_t const willRetain = 0;
uint8_t const TempSensorStringLen = 4;
bool RetainMessage = true;

// MQTT Broker
IPAddress const MQTT_SERVER(192, 168, 1, 254);

// The IP address of the Arduino
uint8_t const mac[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

// Remove remarks and alter if you want a fixed IP for the Arduino
IPAddress const ip(192, 168, 1, 200);
IPAddress const dns_gateway(192, 168, 0, 3);
IPAddress const subnet(255,255,254,0);


// Delay timer
static uint32_t queryClimateTimer;
static uint32_t StateCheck = 0;
static unsigned long previous;
static char prevTemp[TempSensorStringLen] = "UwU"; // Init with impossible value to trigger publish on boot
static char prevHumi[TempSensorStringLen] = "UwU"; // Init with impossible value to trigger publish on boot
static StringIndex_t prevDoorStatus = String_IndexCount; // Init with impossible value to trigger publish on boot

// Ethernet Initialisation
EthernetClient ethClient;
GarageDoorState garageDoorState;
StateChanger stateChanger;
WebServer webServer;

AM232X AM2320;

static bool PublishMQTTMessage (StringIndex_t sMQTTSubscription, char const * const sMQTTData, bool retain = false);

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
  wdt_enable(WDTO_2S);
  Serial.begin(115200);
  char szString[StringTable_SingleStringMaxLength];
  getString(String_HelloWorld, szString);
  webServer.placeString(szString);

  queryClimateTimer = 0;

  stateChanger.bindGarageDoorState(&garageDoorState);
  stateChanger.bindWebServer(&webServer);
  garageDoorState.bindWebServer(&webServer);

  previous = 0;

  // Start Network (replace with 'Ethernet.begin(mac, ip);' for fixed IP)
  Ethernet.begin(const_cast<uint8_t *>(mac), ip, dns_gateway, dns_gateway, subnet);
  webServer.placeString("Ethernet init" LINE_BREAK);
  // Digital IO 17 act as GND for AM2320B sensor
  pinMode(AM2320_PINGND, OUTPUT);
  digitalWrite(AM2320_PINGND, LOW);
  // Digital IO 16 act as +5V for AM2320B sensor
  pinMode(AM2320_PINVCC, OUTPUT);
  digitalWrite(AM2320_PINVCC, HIGH);

  delay(100); // Stabilize AM2320 power supply
  Wire.begin();
  AM2320.begin();
  AM2320.wakeUp();
  webServer.placeString("Temp/Hum sensor init" LINE_BREAK);
  wdt_reset();
  // Let network have a chance to start up
  delay(1500);
  wdt_reset();
}

void loop (void) 
{
  // If not MQTT connected, try connecting
  bool publishOnBoot = true;
  
  if(!mqttClient.connected())  
  {
    // Connect to MQTT broker on the openhab server, retry constantly
    char szString[StringTable_SingleStringMaxLength];
    char szUserPass[StringTable_MQTTStringMaxLength];
    char szPubAvail[StringTable_MQTTStringMaxLength];
    char szUnavailPayload[StringTable_MQTTStringMaxLength];
    getString(String_PorteDeGarage, szString);
    getString(MQTTBrokerrUser, szUserPass);
    getString(MQTTPubAvailable, szPubAvail);
    getString(MQTTUnavailablePayload, szUnavailPayload);
    while(mqttClient.connect(szString, szUserPass, szUserPass, szPubAvail, willQos, willRetain, szUnavailPayload) != 1) 
    {
      char szErrorCode[10];
      getString(String_ErrorConnectMQTTState, szString);
      webServer.placeString(szString);
      snprintf(szErrorCode, 10, "%d", (char)mqttClient.state());
      webServer.placeString(szErrorCode);
      webServer.placeString(")" LINE_BREAK);
      delay(1000);
      wdt_reset();
    }
    
    // Subscribe to the activate switch on OpenHAB
    getString(MQTTSubDoorSwitch, szString);
    mqttClient.subscribe(szString);

    if(publishOnBoot)
    {
      // Publish only once, if successfully published.
      getString(String_BootMessage, szString);
      publishOnBoot = PublishMQTTMessage(MQTTPubDoorBoot, szString, RetainMessage) ? false : true;
    }

    // Publish online
    getString(MQTTAvailablePayload, szString);
    PublishMQTTMessage(MQTTPubAvailable, szString);
    getString(String_Pub, szString);
    webServer.placeString(szString);
    getString(MQTTAvailablePayload, szString);
    webServer.placeString(szString);
    webServer.placeString(LINE_BREAK);
  }

  if (garageDoorState.getActualDoorStateChanged())
  {
    char szString[StringTable_SingleStringMaxLength];
    getString(garageDoorState.getActualDoorString(), szString);
    if(PublishMQTTMessage(MQTTPubDoorDoor, szString, RetainMessage))
    {
      getString(String_PubActualDoorState, szString);
      webServer.placeString(szString);
      getString(garageDoorState.getActualDoorString(), szString);
      webServer.placeString(szString);
      webServer.placeString(LINE_BREAK);
      garageDoorState.clearActualDoorStateChanged();
    }
  }
  
  StringIndex_t doorStatus = garageDoorState.getDoorString();
  if(prevDoorStatus != doorStatus)
  {
    prevDoorStatus = doorStatus;
    char szString[StringTable_SingleStringMaxLength];
    getString(doorStatus, szString);
    PublishMQTTMessage(MQTTPubDoorStatus, szString, RetainMessage);
    getString(String_PubDoorState, szString);
    webServer.placeString(szString);
    getString(doorStatus, szString);
    webServer.placeString(szString);
    webServer.placeString(LINE_BREAK);
  }

  if(0 == queryClimateTimer)
  {
    queryClimateTimer = QueryClimateInterval_ms;

    if(AM232X_OK == AM2320.read())
    {
      float const temp = AM2320.getTemperature();
      float const hum = AM2320.getHumidity();

      char format[TempSensorStringLen + 1]; // +1 for null-terminating char

      dtostrf(temp, TempSensorStringLen, 1, format);
      if(memcmp(format, prevTemp, TempSensorStringLen))
      {
        memcpy(prevTemp, format, TempSensorStringLen);
        PublishMQTTMessage(MQTTPubTemperature, format, RetainMessage);
        char szString[StringTable_SingleStringMaxLength];
        getString(String_Pub, szString);
        webServer.placeString(szString);
        webServer.placeString(format);
        webServer.placeString(LINE_BREAK);
      }

      dtostrf(hum, TempSensorStringLen, 1, format);
      if(memcmp(format, prevHumi, TempSensorStringLen))
      {
        memcpy(prevHumi, format, TempSensorStringLen);
        PublishMQTTMessage(MQTTPubHumidity, format, RetainMessage);  
        char szString[StringTable_SingleStringMaxLength];
        getString(String_Pub, szString);
        webServer.placeString(szString);
        webServer.placeString(format);
        webServer.placeString(LINE_BREAK);
      }
    }
  }
  // Do it all over again
  mqttClient.loop();

  advanceTimers();

  if(0 == StateCheck)
  {
    garageDoorState.run(StateCheckInterval_ms); // Must be ran at every 1000ms interval for now...
    StateCheck = StateCheckInterval_ms;
    wdt_reset();
  }

  stateChanger.run();

  webServer.run();
}

// Publish MQTT data to MQTT broker
static bool PublishMQTTMessage (StringIndex_t sMQTTSubscription, char const * const sMQTTData, bool retain)
{
  // Define and send message about door state
  char szString[StringTable_MQTTStringMaxLength];
  getString(sMQTTSubscription, szString);
  bool publishSuccess = mqttClient.publish(szString, sMQTTData, retain); 

  // Debug info
  webServer.placeString("Outbound: ");
  webServer.placeString(szString);
  webServer.placeString(":");
  webServer.placeString(sMQTTData);
  webServer.placeString(LINE_BREAK);
 
 return publishSuccess;
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

    if(queryClimateTimer)
    {
      queryClimateTimer--;
    }
  }
}
