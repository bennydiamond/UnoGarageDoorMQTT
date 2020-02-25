#include "WebServer.h"
#include "strings.h"

// Syslog server connection info
IPAddress const SYSLOG_SERVER(192, 168, 0, 254) PROGMEM;
#define SYSLOG_PORT 514
// This device info
#define DEVICE_HOSTNAME "garagedooropener"
#define APP_NAME "garagedooropener"

WebServer::WebServer() :
server(80),
ringBuf(),
udpClient(),
syslog(udpClient, SYSLOG_SERVER, SYSLOG_PORT, DEVICE_HOSTNAME, APP_NAME, LOG_KERN, SYSLOG_PROTO_IETF)
{
}

WebServer::~WebServer()
{
}

void WebServer::run (void)
{
EthernetClient client = server.available();
  if (client) 
  {
    Serial.println(String_NewWebClient);
    syslog.log(String_NewWebClient);
    // an http request ends with a blank line
    bool currentLineIsBlank = true;
    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) 
        {
          // send a standard http response header
          //client.println("HTTP/1.1 200 OK");
          //client.println("Content-Type: text/html");
          //client.println("Connection: close");  // the connection will be closed after completion of the response
          //client.println("Refresh: 5");  // refresh the page automatically every 5 sec
          //client.println();
          //client.println("<!DOCTYPE HTML>");
          //client.println("<html>");

            char character;
            while(ringBuf.pop(character))
            {
                //if('\n' == character)
                {
                //  client.print("<br />");
                }
                //else
                {
                  client.print(character);
                }
            }

          //client.println("</html>");
          break;
        }
        if (c == '\n') 
        {
          // you're starting a new line
          currentLineIsBlank = true;
        } else if (c != '\r') 
        {
          // you've gotten a character on the current line
          currentLineIsBlank = false;
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}

void WebServer::placeString (char const * const szIn, bool debug)
{
    uint16_t const len = strlen(szIn);

    Serial.print(szIn);
    if(false == debug)
    {
      syslog.log(szIn);
    }

    for(uint16_t i = 0; i < len; i++)
    {
        if(ringBuf.isFull())
        {
            char dummy;
            ringBuf.pop(dummy);
        }
        
        ringBuf.push(szIn[i]);
    }
}