
#include <Arduino.h>
#include <Ethernet.h>
#include <RingBuf.h>
#include <EthernetUdp.h>
#include <Syslog.h>
#include <string.h>
#include <stdint.h>

#pragma once

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void run (void);

    void placeString (char const * const szIn, bool debug = true);

private:
    #define logBufferSize 200

    EthernetServer server;
    RingBuf<char, logBufferSize> ringBuf;
    EthernetUDP udpClient;
    Syslog syslog;
};