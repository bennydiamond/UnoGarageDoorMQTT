#include "WebServer.h"

WebServer::WebServer() :
server(80),
ringBuf()
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
    Serial.println("new client");
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

void WebServer::placeString (char const * const szIn)
{
    uint16_t const len = strlen(szIn);

    Serial.print(szIn);

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