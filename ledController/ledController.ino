#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <string>

MDNSResponder mdns;

const char* ssid = "a_r_o_2";
const char* password = "Cern0wiec";

unsigned long epochTime;
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


// outputs
int dout1 = 4;
int dout2 = 5;
int doutled = 2;
boolean out1 = false;
boolean out2 = false;

ESP8266WebServer server(80);
String webPage = "";

void setup(void){
  updateWebPageBody();
  
  // preparing GPIOs
  pinMode(dout1, OUTPUT);
  digitalWrite(dout1, LOW);
  pinMode(dout2, OUTPUT);
  digitalWrite(dout2, LOW);
  pinMode(doutled, OUTPUT);
  digitalWrite(doutled, LOW);

  uint8_t macAddress[6];
  WiFi.macAddress(macAddress);
  delay(100);
  Serial.printf("\n\nMAC address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                macAddress[0],
                macAddress[1],
                macAddress[2],
                macAddress[3],
                macAddress[4],
                macAddress[5]);
           
  delay(1000);
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    blinkStatusLED(250, 250);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", [](){
    server.send(200, "text/html", webPage);
  });
  server.on("/socket1On", [](){
    digitalWrite(dout1, HIGH);
    blinkStatusLED(200,200);
    out1 = !out1;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.print("Internal clock for reference: ");
    Serial.println(millis());
  });
  server.on("/socket1Off", [](){
    digitalWrite(dout1, LOW);
    out1 = !out1;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
  });
  server.on("/socket2On", [](){
    digitalWrite(dout2, HIGH);
    //delay(1000);
    blinkStatusLED(200,200);
    out2 = !out2;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.print("Internal clock for reference: ");
    Serial.println(millis());    
  });
  server.on("/socket2Off", [](){
    digitalWrite(dout2, LOW);
    out2 = !out2;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
  });

  server.on("/allOn", [](){
    digitalWrite(dout1, HIGH);
    digitalWrite(dout2, HIGH);
    blinkStatusLED(200,200);
    out2 = true;
    out1 = true;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.println("All ON...");
    
  });

   server.on("/allOff", [](){
    digitalWrite(dout1, LOW);
    digitalWrite(dout2, LOW);
    blinkStatusLED(200,200);
    out2 = false;
    out1 = false;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.println("All OFF...");
  });
  
  server.begin();
  Serial.println("HTTP server started");

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
  epochTime = getTime();
  Serial.print("Internal clock for reference: ");
  Serial.println(millis());
}

void blinkStatusLED(int high, int low){
    digitalWrite(doutled, HIGH);
    delay(high);
    digitalWrite(doutled, LOW);
    delay(low);
}
 
void loop(void){
  blinkStatusLED(1000,1000);
  server.handleClient();
} 


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
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


unsigned long getTime(){
   //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP); 
  unsigned long epoch = 1;
  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(2000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);

    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    Serial.print(':');
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    Serial.print(':');
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
    }
    Serial.println(epoch % 60); // print the second
    
  }
  // wait ten seconds before asking for the time again
  //delay(10000);
  return epoch;
}

void updateWebPageBody(){
  webPage = "";
  webPage += "<!DOCTYPE HTML>\r\n<html>\n<head>\n";
  webPage += "<meta charset=\"utf-8\"> \n";
  webPage += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  webPage += "<title>HOME LIGTHS Arduino</title>";
  //bootstrap
  webPage += "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\" ";
  webPage += "integrity=\"sha384-BVYiiSIFeK1dGmJRAkycuHAHRg32OmUcww7on3RYdg4Va+PmSTsz/K68vbdEjh4u\" crossorigin=\"anonymous\"> \n";
  webPage += " <script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\" ";
  webPage += "integrity=\"sha384-Tc5IQib027qvyjSMfHjOMaLkfuWVxZxUPnCJA7l2mCWNIpG9mGCD8wGNIcPD7Txa\" crossorigin=\"anonymous\"></script> \n";
  //
  webPage += "  <link rel=\"stylesheet\" href=\"//code.jquery.com/ui/1.12.0/themes/base/jquery-ui.css\">\n";
  webPage += "  <link rel=\"stylesheet\" href=\"/resources/demos/style.css\">\n";
  webPage += "  <script src=\"https://code.jquery.com/jquery-1.12.4.js\"></script>\n";
  webPage += "  <script src=\"https://code.jquery.com/ui/1.12.0/jquery-ui.js\"></script>\n";
  webPage += "<script>\n";
  webPage += "$( function() {\n";
  webPage  += "   $( \"input\" ).checkboxradio();\n";
  webPage += "} );\n";
  webPage += "</script>\n";
  webPage += "<script>\n";
  webPage += "$( function() {\n";
  webPage  += "   $( \"#accordion\" ).accordion();\n";
  webPage += "} );\n";
  webPage += "</script>\n";
  webPage +=" <head/>\n<body>\n";
  webPage += "<h1>Garden Lights v0.1 </h1>\n";

  //Status
  webPage += "<div id=\"accordion\">";

  // Controll
  
  webPage += "<h2>Controll: </h2>\n<div>\n";
  webPage += "<p> "; 
    if (out1){
    webPage += " <a href=\"socket1Off\"><button class=\"btn btn-danger btn-lg\">OFF Lights 1 </button></a>\n";
  }else{
    webPage += " <a href=\"socket1On\"> <button class=\"btn btn-success btn-lg\">ON Lights 1 </button></a>\n ";
  }
  webPage += "</p> \n";
  webPage += "<p> "; 
    if (out2){
    webPage += " <a href=\"socket2Off\"><button class=\"btn btn-danger btn-lg\">OFF Lights 2 </button></a>\n";
  }else{
    webPage += " <a href=\"socket2On\"> <button class=\"btn btn-success btn-lg\">ON Lights 2 </button></a>\n ";
  }
  webPage += "</p> \n";
  webPage += "<p> <a href=\"allOn\">  <button class=\"btn btn-success btn-lg\">ON ALL</button></a> &nbsp; <a href=\"allOff\">  <button class=\"btn btn-danger btn-lg\">OFF ALL</button></a></p> \n";  
  webPage += "</div>\n";
  
  webPage += "<h2>Status: </h2>\n<div>\n";
  char temp[50];
  sprintf(temp, "<p>Start systemu: %lu </p>", epochTime);
  webPage += temp; 
  if (out1){
    webPage += " <p class=\"bg-success\">Lights 1 are ON</p>\n";
  }else{
    webPage += " <p class=\"bg-danger\">Lights 1 are OFF</p>\n";
  }
  if (out2){
    webPage += " <p class=\"bg-success\">Lights 2 are ON </p>\n";
  }else{
    webPage += " <p class=\"bg-danger\">Lights 2 are OFF</p>\n";
  }
  webPage += "</div>\n";

  webPage += "<h2>Configuration: </h2>\n<div>\n"; 
  webPage += "<fieldset>\n";
  webPage += "  <legend>Automatic swichoff</legend>\n";
  webPage += "  <label for=\"checkbox-1\">Mon - Fri</label>\n";
  webPage += "  <input type=\"checkbox\" name=\"checkbox-1\" id=\"checkbox-1\">\n";
  webPage += "  <label for=\"checkbox-2\">Sat</label>\n";
  webPage += "  <input type=\"checkbox\" name=\"checkbox-2\" id=\"checkbox-2\">\n";
  webPage += "  <label for=\"checkbox-3\">Sun</label>\n";
  webPage += "  <input type=\"checkbox\" name=\"checkbox-3\" id=\"checkbox-3\">\n";
  webPage += "</fieldset>\n";
  webPage += " <a href=\"ScheduleSave\"><button class=\"btn btn-success btn-lg\">Save changes</button></a>\n";
  webPage += "</div>\n";
  
  webPage += "</div>\n";
  webPage += "</body>\n</html>\n";
}

