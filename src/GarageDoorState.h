
#include "DoorStates.h"
#include "WebServer.h"
#include "strings.h"
#include <stdint.h>

#pragma once

class GarageDoorState
{
public:
    GarageDoorState ();
    ~GarageDoorState ();

    void run (uint32_t elapsedTime_ms);

    DoorState_t getCurrentDoorState (void) { return reportedDoorState; }
    DoorState_t getExpectedNextDoorState (void) { return targetDoorState; }
    bool getDoorStoppedMid (void) { return doorStoppedMid; }
    bool getStateReachFailed (void) { return stateReachFailed; }
    void forceResetStateReachFailed (void) { stateReachFailed = false; timeoutCounter = -1; }
    StringIndex_t getDoorString (void);
    void relayIsToggled (void);

    bool getActualDoorStateChanged (void) { return actualDoorStateChanged; }
    void clearActualDoorStateChanged (void) { actualDoorStateChanged = false; }
    StringIndex_t getActualDoorString (void);

    void bindWebServer (WebServer * in) { webServer = in; }

private:
    static uint8_t const TransitionTimeoutCount_s PROGMEM;
    static uint8_t const StateHoldTimeCount PROGMEM;
    void checkCurrentState (void);
    static void updateActualDoorState (void);

    // Initial door state (assume neither closed nor open when starting)
    DoorState_t lastKnownDoorState;
    DoorState_t targetDoorState;
    DoorState_t previousTargetDoorState;
    DoorState_t reportedDoorState;
    bool doorStoppedMid;
    int8_t timeoutCounter;
    uint8_t stateHoldCounter;
    bool stateHoldReported;
    bool stateReachFailed;
    bool relayActivated;
    bool inTransition;

    volatile DoorState_t actualDoorState;
    volatile bool actualDoorStateChanged;

    WebServer * webServer;
};