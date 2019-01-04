#include "GarageDoorState.h"
#include <Arduino.h>

#define PINOPEN 5     // Door Open PIN - Reed switch for door open
#define PINCLOSED 6   // Door Closed PIN - Reed switch for door shut

uint8_t const GarageDoorState::TransitionTimeoutCount = 20;
uint8_t const GarageDoorState::StateHoldTimeCount = 3;

char const * const DoorStateStrings[] =
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
  // Reed switch pins are inputs, activate internal pullup resistor by writing high
  pinMode(PINCLOSED, INPUT_PULLUP);
  digitalWrite(PINCLOSED, HIGH);
  pinMode(PINOPEN, INPUT_PULLUP);
  digitalWrite(PINOPEN, HIGH);

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
            char count[3];
            webServer->placeString("Timeout: ");
            snprintf(count, 3, "%d", timeoutCounter);
            webServer->placeString(count);
            webServer->placeString(LINE_BREAK);
            webServer->placeString("State not reached." LINE_BREAK);
        }

        if(targetDoorState == lastKnownDoorState && false == inTransition)
        {
            // Arrived at destination
            if(StateHoldTimeCount < stateHoldCounter)
            {
                timeoutCounter = -1;
                stateReachFailed = false;
                webServer->placeString("State reached." LINE_BREAK);
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
            webServer->placeString("State reach failed." LINE_BREAK);
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

void GarageDoorState::relayIsToggled (void)
{
    timeoutCounter = TransitionTimeoutCount;
    stateReachFailed = false;
    relayActivated = true;
    stateHoldCounter = 0;

    // If stopped
    if(inTransition)
    {
        webServer->placeString("Door Stopped mid" LINE_BREAK);
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
      webServer->placeString("Sensor Error" LINE_BREAK);
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

    webServer->placeString("LastKnownState: ");
    webServer->placeString(DoorStateStrings[lastKnownDoorState]);
    webServer->placeString(LINE_BREAK);
    webServer->placeString("TargetState: ");
    webServer->placeString(DoorStateStrings[targetDoorState]);
    webServer->placeString(LINE_BREAK);
}
