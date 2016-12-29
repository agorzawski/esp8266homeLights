#ifndef TimeService_h
#define TimeService_h
#include "Arduino.h"
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#define NTP_PACKET_SIZE 48

class TimeService
{


  public:
    TimeService();
    static String timeToString(unsigned long epoch);
    unsigned long getTime();

  private:
    unsigned long sendNTPpacket(IPAddress& address);
    unsigned int localPort = 2390;      // local port to listen for UDP packets

    IPAddress timeServerIP; // time.nist.gov NTP server address
    char* ntpServerName = "time.nist.gov";
    //int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

    byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
    WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
};

#endif
