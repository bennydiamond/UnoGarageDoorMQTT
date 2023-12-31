#include "strings.h"

char const String_HelloWorld_[] PROGMEM            = "Hello World!" LINE_BREAK;
char const String_PorteDeGarage_[] PROGMEM         = "Porte de garage";
char const String_Pub_[] PROGMEM                   = "Pub: ";
char const String_PubDoorState_[] PROGMEM          = "Pub Door state: ";
char const String_PubActualDoorState_[] PROGMEM    = "Pub Actual Door state: ";
char const String_ErrorConnectMQTTState_[] PROGMEM = "Error connecting to MQTT (State:";
char const String_NewWebClient_[] PROGMEM          = "new web client";
char const String_StateNotReached_[] PROGMEM       = "State not reached." LINE_BREAK;
char const String_StateReached_[] PROGMEM          = "State reached." LINE_BREAK;
char const String_StateReachFailed_[] PROGMEM      = "State reach failed." LINE_BREAK;
char const String_DoorStoppedMid_[] PROGMEM        = "Door Stopped mid" LINE_BREAK;
char const String_SensorError_[] PROGMEM           = "Sensor Error" LINE_BREAK;
char const String_DoorStateUnknown_[] PROGMEM      = "Unknown" LINE_BREAK;
char const String_DoorStateClose_[] PROGMEM        = "Close" LINE_BREAK;
char const String_DoorStateOpen_[] PROGMEM         = "Open" LINE_BREAK;

char const MQTTSubDoorSwitch_[] PROGMEM      = "garage/door/switch";
char const MQTTPubDoorStatus_[] PROGMEM      = "garage/door/status";
char const MQTTPubDoorDoor_[] PROGMEM        = "garage/door/door";
char const MQTTPubAvailable_[] PROGMEM       = "garage/door/available";
char const MQTTPubTemperature_[] PROGMEM     = "garage/door/temperature";
char const MQTTPubHumidity_[] PROGMEM        = "garage/door/humidity";
char const MQTTBrokerrUser_[] PROGMEM        = "sonoff";
char const MQTTAvailablePayload_[] PROGMEM   = "online";
char const MQTTUnavailablePayload_[] PROGMEM = "offline";

char const * const String_table[String_IndexCount] PROGMEM = 
{
  String_HelloWorld_,
  String_PorteDeGarage_,
  String_Pub_,
  String_PubDoorState_,
  String_PubActualDoorState_,
  String_ErrorConnectMQTTState_,
  String_NewWebClient_,
  String_StateNotReached_,
  String_StateReached_,
  String_StateReachFailed_,
  String_DoorStoppedMid_,
  String_SensorError_,
  String_DoorStateUnknown_,
  String_DoorStateClose_,
  String_DoorStateOpen_,

  MQTTSubDoorSwitch_,
  MQTTPubDoorStatus_,
  MQTTPubDoorDoor_,
  MQTTPubAvailable_,
  MQTTPubTemperature_,
  MQTTPubHumidity_,
  MQTTBrokerrUser_,
  MQTTAvailablePayload_,
  MQTTUnavailablePayload_
};

void getString (StringIndex_t idx, char *out) 
{
  strcpy_P(out, (char *)pgm_read_ptr(&(String_table[idx])));  // Necessary casts and dereferencing, just copy.
}