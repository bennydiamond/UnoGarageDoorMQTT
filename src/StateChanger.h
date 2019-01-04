#include "GarageDoorState.h"
#include "WebServer.h"

#pragma once

class StateChanger
{
public:
    StateChanger();
    ~StateChanger();
    void bindGarageDoorState (GarageDoorState * const in) { garageDoorState = in; }
    void bindWebServer (WebServer * in) { webServer = in; }

    void run (void);
    void stateChangeRequest (DoorState_t newDoorState);

    void toggleRelay (void);
private:
    static uint16_t const retryTimeout_ms;
    static uint8_t const MaxRetryCount;

    int16_t retryTimeout;
    uint8_t retryCount;
    GarageDoorState * garageDoorState;
    WebServer * webServer;
};