#define LINE_BREAK "\r\n"
#include <Arduino.h>

#pragma once

#define StringTable_SingleStringMaxLength (33) // Make sure this is actually true before compiling.
#define StringTable_MQTTStringMaxLength (24) // Make sure this is actually true before compiling.
typedef enum e_StringIndex_t
{
  String_HelloWorld = 0,
  String_PorteDeGarage,
  String_Pub,
  String_PubDoorState,
  String_PubActualDoorState,
  String_ErrorConnectMQTTState,
  String_NewWebClient,
  String_StateNotReached,
  String_StateReached,
  String_StateReachFailed,
  String_DoorStoppedMid,
  String_SensorError,
  String_DoorStateUnknown,
  String_DoorStateClose,
  String_DoorStateOpen,

  // Define Sub's and Pub's
  MQTTSubDoorSwitch,
  MQTTPubDoorStatus,
  MQTTPubDoorDoor,
  MQTTPubAvailable,
  MQTTPubTemperature,
  MQTTPubHumidity,
  MQTTBrokerrUser,
  MQTTAvailablePayload,
  MQTTUnavailablePayload,

  String_IndexCount
} StringIndex_t;
#define MQTTBrokerrPass MQTTBrokerrUser
extern char const * const String_table[String_IndexCount] PROGMEM;


void getString (StringIndex_t idx, char *out);