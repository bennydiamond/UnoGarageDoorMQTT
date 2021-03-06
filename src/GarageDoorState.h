
#include "DoorStates.h"
#include "WebServer.h"
#include <stdint.h>

#pragma once

class GarageDoorState
{
public:
    GarageDoorState ();
    ~GarageDoorState ();

    void run (void);

    DoorState_t getCurrentDoorState (void) { return reportedDoorState; }
    DoorState_t getExpectedNextDoorState (void) { return targetDoorState; }
    bool getDoorStoppedMid (void) { return doorStoppedMid; }
    bool getStateReachFailed (void) { return stateReachFailed; }
    void forceResetStateReachFailed (void) { stateReachFailed = false; timeoutCounter = -1; }
    char const * const getDoorString (void);
    void relayIsToggled (void);

    void bindWebServer (WebServer * in) { webServer = in; }

private:
    static uint8_t const TransitionTimeoutCount;
    static uint8_t const StateHoldTimeCount;
    void checkCurrentState (void);

    // Initial door state (assume neither closed nor open when starting)
    DoorState_t lastKnownDoorState;
    DoorState_t targetDoorState;
    DoorState_t reportedDoorState;
    bool doorStoppedMid;
    int8_t timeoutCounter;
    uint8_t stateHoldCounter;
    bool stateReachFailed;
    bool relayActivated;
    bool inTransition;

    WebServer * webServer;
};