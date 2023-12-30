
#include <Arduino.h>
#include <Ethernet.h>
#include <RingBuf.h>
#include <EthernetUdp.h>
#include <Syslog.h>
#include <string.h>
#include <stdint.h>

#pragma once

// Uncomment to enable remote syslog
//#define ENABLE_SYSLOG

class WebServer
{
public:
    WebServer();
    ~WebServer();

    void run (void);

    void placeString (char const * const szIn, bool debug = true);

private:
    #define logBufferSize 240

    EthernetServer server;
    RingBuf<char, logBufferSize> ringBuf;
    EthernetUDP udpClient;
#ifdef ENABLE_SYSLOG
    Syslog syslog;
#endif
};