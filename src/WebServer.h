
#include <Arduino.h>
#include <Ethernet.h>
#include <RingBuf.h>
#include <string.h>
#include <stdint.h>

#define LINE_BREAK "\r\n"

#pragma once

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void run (void);

    void placeString (char const * const szIn);

private:
    #define logBufferSize 240

    EthernetServer server;
    RingBuf<char, logBufferSize> ringBuf;
};