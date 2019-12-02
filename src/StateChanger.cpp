#include "StateChanger.h"
#include "pins.h"
#include <Arduino.h>
#include <stddef.h>

uint16_t const StateChanger::retryTimeout_ms = 3000;
uint8_t const StateChanger::MaxRetryCount = 5;

StateChanger::StateChanger() :
retryTimeout(-1),
retryCount(0),
garageDoorState(NULL)
{
    pinMode(PINLED, OUTPUT);
    pinMode(PINRELAY, OUTPUT);
    digitalWrite(PINLED, LOW);
    digitalWrite(PINRELAY, LOW);
}

StateChanger::~StateChanger()
{

}

void StateChanger::run (void)
{
    if(MaxRetryCount <= retryCount)
    {
        webServer->placeString("Max Retry Count, aborting." LINE_BREAK);
        garageDoorState->forceResetStateReachFailed();
        retryCount = 0;
    }
    else if(garageDoorState->getStateReachFailed())
    {
        retryCount++;
        toggleRelay();
    }
}

void StateChanger::stateChangeRequest (DoorState_t newDoorState)
{
    bool toggleRelayApproved = false;
    DoorState_t deductedDoorState = garageDoorState->getCurrentDoorState();
    switch(newDoorState)
    {
    default:
    //Intentionnal fall-through
    case DoorState_Close:
        if(DoorState_Close != deductedDoorState)
        {
            toggleRelayApproved = true;
        }
    break;
    case DoorState_Open:
        if(DoorState_Open != deductedDoorState)
        {
            toggleRelayApproved = true;
        }
    break;
    }

    if(toggleRelayApproved)
    {
        toggleRelay();
        retryCount = 0;   
    }
    else
    {
        webServer->placeString("NOT Activating Relay" LINE_BREAK);
    }
}

void StateChanger::toggleRelay (void)
{
  webServer->placeString("Activating Relay" LINE_BREAK);
  digitalWrite(PINRELAY, HIGH);
  digitalWrite(PINLED, LOW);
  delay(1000); 
  digitalWrite(PINRELAY, LOW);
  digitalWrite(PINLED, HIGH);        
  garageDoorState->relayIsToggled();
  retryTimeout = millis() + retryTimeout_ms;
}
