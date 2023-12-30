#include "GarageDoorState.h"
#include "pins.h"
#include "strings.h"
#include <Arduino.h>

uint8_t const GarageDoorState::TransitionTimeoutCount PROGMEM = 20;
uint8_t const GarageDoorState::StateHoldTimeCount PROGMEM = 3;
static GarageDoorState* garageDoorStateImpl = nullptr;

char const * const DoorStateStrings[] PROGMEM =
{
  [DoorState_Unknown] = "Unknown",
  [DoorState_Open]    = "Open",
  [DoorState_Close]   = "Close"
};

GarageDoorState::GarageDoorState () :
lastKnownDoorState(DoorState_Unknown),
targetDoorState(DoorState_Unknown),
doorStoppedMid(false),
timeoutCounter(-1),
stateHoldCounter(0),
stateReachFailed(false),
relayActivated(false),
inTransition(false)
{
  garageDoorStateImpl = this;
  // HAL effect sensor pins are inputs, activate internal pullup resistor by writing high
  pinMode(PINCLOSED, INPUT_PULLUP);
  digitalWrite(PINCLOSED, HIGH);
  pinMode(PINOPEN, INPUT_PULLUP);
  digitalWrite(PINOPEN, HIGH);

  // Digital IO 3 act as GND for Reed switch
  pinMode(REED_SINK_GND, OUTPUT);
  digitalWrite(REED_SINK_GND, LOW);

  // Reed switch input for actual door state, activate internal pullup resistor by writing high
  pinMode(REED_IO_PULLUP, INPUT_PULLUP);
  digitalWrite(REED_IO_PULLUP, HIGH);
  updateActualDoorState();
  attachInterrupt(digitalPinToInterrupt(REED_IO_PULLUP), updateActualDoorState, CHANGE);

  checkCurrentState();
  targetDoorState = lastKnownDoorState;
}
GarageDoorState::~GarageDoorState ()
{

}

// Call every 3 seconds
void GarageDoorState::run (void)
{
    if(false == relayActivated)
    {
        checkCurrentState();
        if(-1 != timeoutCounter)
        {
            char sz[20];
            snprintf(sz, 128, "Timeout:  %d" LINE_BREAK, timeoutCounter);
            webServer->placeString(sz);
            webServer->placeString(String_StateNotReached);
        }

        if(targetDoorState == lastKnownDoorState && false == inTransition)
        {
            // Arrived at destination
            if(StateHoldTimeCount < stateHoldCounter)
            {
                timeoutCounter = -1;
                stateReachFailed = false;
                webServer->placeString(String_StateReached);
            }
            else
            {
                stateHoldCounter++;
            }
        }
        else if(0 < timeoutCounter)
        {
            timeoutCounter--;
            stateHoldCounter = 0;
        }

        if(0 == timeoutCounter)
        {
            webServer->placeString(String_StateReachFailed);
            stateReachFailed = true;
            stateHoldCounter = 0;
        }
    }
    relayActivated = false;
}

char const * const GarageDoorState::getDoorString (void) 
{ 
    return DoorStateStrings[reportedDoorState]; 
}

char const * const GarageDoorState::getActualDoorString (void) 
{ 
    return DoorStateStrings[actualDoorState]; 
}

void GarageDoorState::relayIsToggled (void)
{
    timeoutCounter = TransitionTimeoutCount;
    stateReachFailed = false;
    relayActivated = true;
    stateHoldCounter = 0;

    // If stopped
    if(inTransition)
    {
        webServer->placeString(String_DoorStoppedMid);
        doorStoppedMid = false;
        if(DoorState_Close == targetDoorState)
        {
            targetDoorState = DoorState_Open;
        }
        else if(DoorState_Open == targetDoorState)
        {
            targetDoorState = DoorState_Close;
        }
    }
    // If at stable state
    else if(DoorState_Close == lastKnownDoorState)
    {
        targetDoorState = DoorState_Open;
    }
    else if(DoorState_Open == lastKnownDoorState)
    {
        targetDoorState = DoorState_Close;
    }
    // If currently in transition
}

void GarageDoorState::checkCurrentState (void)
{
    uint8_t closeSensor = digitalRead(PINCLOSED) == LOW ? 1 : 0;
    uint8_t openSensor = digitalRead(PINOPEN) == LOW ? 1 : 0;
    inTransition = false;

    reportedDoorState = DoorState_Open;
    
    if(openSensor && closeSensor)
    {
      // Both sensors show true at the same time... Error!
      webServer->placeString(String_SensorError);
      lastKnownDoorState = DoorState_Unknown;
    }
    else if(openSensor) 
    {
      lastKnownDoorState = DoorState_Open;
      doorStoppedMid = false;
    }
    else if(closeSensor) 
    {
      lastKnownDoorState = DoorState_Close;
      reportedDoorState = DoorState_Close;
      doorStoppedMid = false;
    }
    else
    {
        inTransition = true;
    }

    if(false == inTransition)
    {
        if(DoorState_Unknown == targetDoorState)
        {
            targetDoorState = lastKnownDoorState;
        }
    }

    char sz[30];
    snprintf(sz, 30, "LastKnownState: %s" LINE_BREAK, DoorStateStrings[lastKnownDoorState]);
    webServer->placeString(sz);
    snprintf(sz, 30, "TargetState: %s" LINE_BREAK, DoorStateStrings[targetDoorState]);
    webServer->placeString(sz);
}

void GarageDoorState::updateActualDoorState (void)
{
    garageDoorStateImpl->actualDoorState = digitalRead(REED_IO_PULLUP) == LOW ? DoorState_Close : DoorState_Open;
    garageDoorStateImpl->actualDoorStateChanged = true;
}
