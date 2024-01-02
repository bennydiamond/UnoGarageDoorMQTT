#include "GarageDoorState.h"
#include "pins.h"
#include <Arduino.h>

uint8_t const GarageDoorState::TransitionTimeoutCount_s = 20;
uint8_t const GarageDoorState::StateHoldTimeCount = 3;
static GarageDoorState* garageDoorStateImpl = nullptr;

StringIndex_t DoorStateStrings[] =
{
  [DoorState_Unknown] = String_DoorStateUnknown,
  [DoorState_Open]    = String_DoorStateOpen,
  [DoorState_Close]   = String_DoorStateClose
};

GarageDoorState::GarageDoorState () :
lastKnownDoorState(DoorState_Unknown),
targetDoorState(DoorState_Unknown),
previousTargetDoorState(DoorState_Open), // init value must be different from member `targetDoorState`
doorStoppedMid(false),
timeoutCounter(-1),
stateHoldCounter(0),
stateHoldReported(false),
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
  actualDoorState = digitalRead(REED_IO_PULLUP) == LOW ? DoorState_Close : DoorState_Open;
  actualDoorStateChanged = true;
  attachInterrupt(digitalPinToInterrupt(REED_IO_PULLUP), updateActualDoorState, CHANGE);

  checkCurrentState();
  targetDoorState = lastKnownDoorState;
}
GarageDoorState::~GarageDoorState ()
{

}

// Call every 3 seconds
void GarageDoorState::run (uint32_t elapsedTime_ms)
{
    if(false == relayActivated)
    {
        checkCurrentState();
        if(-1 != timeoutCounter)
        {
            char sz[20];
            snprintf(sz, 128, "Timeout:  %d" LINE_BREAK, timeoutCounter);
            webServer->placeString(sz);
            char szString[StringTable_SingleStringMaxLength];
            getString(String_StateNotReached, szString);
            webServer->placeString(szString);
        }

        if(targetDoorState == lastKnownDoorState && false == inTransition)
        {
            // Arrived at destination
            if(StateHoldTimeCount < stateHoldCounter)
            {
                timeoutCounter = -1;
                stateReachFailed = false;
                if(false == stateHoldReported)
                {
                    stateHoldReported = true;
                    char szString[StringTable_SingleStringMaxLength];
                    getString(String_StateReached, szString);
                    webServer->placeString(szString);
                }
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
            stateHoldReported = false;
        }

        if(0 == timeoutCounter)
        {
            if(false == stateHoldReported)
            {
                stateHoldReported = true;
                char szString[StringTable_SingleStringMaxLength];
                getString(String_StateReachFailed, szString);
                webServer->placeString(szString);
            }
            stateReachFailed = true;
            stateHoldCounter = 0;
        }
    }
    relayActivated = false;
}

StringIndex_t GarageDoorState::getDoorString (void) 
{ 
    return DoorStateStrings[reportedDoorState]; 
}

StringIndex_t GarageDoorState::getActualDoorString (void) 
{ 
    return DoorStateStrings[actualDoorState]; 
}

void GarageDoorState::relayIsToggled (void)
{
    timeoutCounter = TransitionTimeoutCount_s;
    stateReachFailed = false;
    relayActivated = true;
    stateHoldCounter = 0;
    stateHoldReported = false;

    // If stopped
    if(inTransition)
    {
        char szString[StringTable_SingleStringMaxLength];
        getString(String_DoorStoppedMid, szString);
        webServer->placeString(szString);
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
    DoorState_t lastKnownDoorStateOnEntry = lastKnownDoorState;

    reportedDoorState = DoorState_Open;
    
    if(openSensor && closeSensor)
    {
      // Both sensors show true at the same time... Error!
      char szString[StringTable_SingleStringMaxLength];
      getString(String_SensorError, szString);
      webServer->placeString(szString);
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

    if(lastKnownDoorStateOnEntry != lastKnownDoorState)
    {
        char sz[30];
        char szString[StringTable_SingleStringMaxLength];
        getString(DoorStateStrings[lastKnownDoorState], szString);
        snprintf(sz, 30, "LastKnownState: %s" LINE_BREAK, szString);
        webServer->placeString(sz);
    }

    if(previousTargetDoorState != targetDoorState)
    {
        previousTargetDoorState = targetDoorState;
        char sz[30];
        char szString[StringTable_SingleStringMaxLength];
        getString(DoorStateStrings[targetDoorState], szString);
        snprintf(sz, 30, "TargetState: %s" LINE_BREAK, szString);
        webServer->placeString(sz);
    }
}

void GarageDoorState::updateActualDoorState (void)
{
    static unsigned long last_interrupt_time = 0;
    unsigned long interrupt_time = millis();
    // If interrupts come faster than 200ms, assume it's a bounce and ignore
    // Garage doors don't move that fast anyway!
    if (interrupt_time - last_interrupt_time > 200) 
    {
        garageDoorStateImpl->actualDoorState = digitalRead(REED_IO_PULLUP) == LOW ? DoorState_Close : DoorState_Open;
        garageDoorStateImpl->actualDoorStateChanged = true;
    }
    last_interrupt_time = interrupt_time;
}
