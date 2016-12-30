#include "Arduino.h"
#include "TimeService.h"

#define DST 1

TimeService::TimeService(){
}

void TimeService::init()
{
  logFromConnection = "";
  logFromConnection += "---=== Starting TimeService ===---\n";
  logFromConnection += "-- Starting UDP\n";
  udp.begin(localPort);
  logFromConnection += "-- Local port: ";
  logFromConnection += String(udp.localPort())+"\n";
  unsigned long epochTime = TimeService::getTime();
  logFromConnection += "-- Current time: \n";
  logFromConnection += timeToString(epochTime, DST)+"\n";
  logFromConnection += "-- Internal clock for the reference: ";
  logFromConnection +=  String(millis())+"\n";
  logFromConnection += "---=== TimeService STARTED ===---\n";
  Serial.println(logFromConnection);
}

String TimeService::getLastLog()
{
  return logFromConnection;
}

unsigned long TimeService::getTime() {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  unsigned long epoch = 1;
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(2000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    logFromConnection += "-- No packet yet!, retrying...\n";
  }
  else {
    logFromConnection += "-- Packet received!\n";
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    logFromConnection += "-- Seconds since Jan 1 1900 = " ;
    logFromConnection += String(secsSince1900) + "\n";

    // now convert NTP time into everyday time:
    logFromConnection += "-- Unix time = ";
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    // print Unix time:
    logFromConnection += String(epoch) + "\n";
    logFromConnection += "-- " + timeToString(epoch, DST) + "\n";
  }
  return epoch;
}

// send an NTP request to the time server at the given address
unsigned long TimeService::sendNTPpacket(IPAddress& address)
{
  logFromConnection += "-- Sending NTP packet...\n";
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

String TimeService::timeToString(unsigned long epoch, int dst){
    String oss = "GMT+" + String(DST) + ": " + String(((epoch  % 86400L) / 3600) + dst) + ":";
    if ( ((epoch % 3600) / 60) < 10 ) {
      oss += "0";
    }
    oss +=  String( ((epoch  % 3600) / 60) ) + ":"; 
    if ( (epoch % 60) < 10 ) {
      oss+= "0";
    }
    oss +=  String((epoch % 60)); 
    return oss; 
}
