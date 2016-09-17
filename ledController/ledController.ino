#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "FS.h"
#include <string>
#include <sstream>

MDNSResponder mdns;

const char* ssid = "a_r_o_2";
const char* password = "Cern0wiec";
unsigned long epochTime;
unsigned long lastCheckEpoch;
int hourOn = 0;
int hourOff = 1;
String daysScheduled = "01111110";

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

// outputs
int dout [] = {4,5,0};
boolean out[] = {false, false, false};
int doutled=2;

ESP8266WebServer server(80);
String webPage = "";
  
void setup(void) {
  updateWebPageBody();

  // preparing GPIOs
  pinMode(dout[0], OUTPUT);
  digitalWrite(dout[0], LOW);
  pinMode(dout[1], OUTPUT);
  digitalWrite(dout[1], LOW);
  pinMode(dout[2], OUTPUT);
  digitalWrite(dout[2], LOW);
  pinMode(doutled, OUTPUT);
  digitalWrite(doutled, LOW);

  delay(1000);
  Serial.begin(115200);

  // Begin connection
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

  //load the configuration 
  SPIFFS.begin();
  File f = SPIFFS.open("/timer.conf", "w");
  if (!f) {
    Serial.println("File open failed!");
    Serial.println("Creating a default one...");
    f.println("01111111");
    f.println("20");
    f.println("2");
    Serial.println("...DONE!");
    f.close();
    f = SPIFFS.open("/timer.conf", "r");
  } else {
    f.close();
    f = SPIFFS.open("/timer.conf", "r");
    Serial.println("Config file exists!");
    Serial.println(f.name());
    int lineCount=0;
    while(f.available()) {
      //Lets read line by line from the file
      String line = f.readStringUntil('\n');
      switch (lineCount){
        case 0 : break;
        case 1 : hourOn = line.toInt(); break;
        case 2 : hourOff = line.toInt(); break;
      }    
      Serial.println(line);
      lineCount++;
    }
  }
  f.close();
  
  // Set HTML server responses
  server.on("/", []() {
    server.send(200, "text/html", webPage);
  });

  server.on("/ScheduleSave", HTTP_GET,  [](){
    String hourOn_s=server.arg("startHour");
    String hourOff_s=server.arg("endHour");
    Serial.println("GET hourOn: " + hourOn_s);
    hourOn = hourOn_s.toInt();
    hourOff = hourOff_s.toInt();

    char* days[] = {"w-1","w-2","w-3","w-4","w-5","w-6","w-7"};
    String configString = "0";
    for (String one: days){
      String state=server.arg(one);
      if (state == "on"){
        configString += "1";
      } else{
        configString += "0";
      }
    }
    daysScheduled = configString;  
    //saveFile(configString, hourOn, hourOff) ;
    updateWebPageBody();
    server.send(200, "text/html", webPage);
  });
  server.on("/socket1On", []() {
    socketOn(0);  });
  server.on("/socket1Off", []() {
    socketOff(0); });
  server.on("/socket2On", []() {
    socketOn(1);  });
  server.on("/socket2Off", []() {
    socketOff(1); });
  server.on("/socket3On", []() {
    socketOn(2);  });
  server.on("/socket3Off", []() {
    socketOff(2); });
  server.on("/allOn", []() {
    for (int i=0; i<sizeof(dout);i++){
      digitalWrite(dout[i], HIGH);
      out[i]=true;
    }
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.println("All ON...");
  });
  server.on("/allOff", []() {
    for (int i=0; i<sizeof(dout);i++){
      digitalWrite(dout[i], LOW);
      out[i]=false;
    }
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.println("All OFF...");
  });

  server.on("/saveSettings", []() {
    // process the form (from POST)
    Serial.println("All SET...");
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

void loop(void) {
  blinkStatusLED(1000, 1000);
  checkTimers();
  server.handleClient();
}

void socketOn(int i){
  digitalWrite(dout[i], HIGH);
  blinkStatusLED(200, 200);
  out[i] = true;
  updateWebPageBody();
  server.send(200, "text/html", webPage);
  Serial.print("Internal clock for reference: ");
  Serial.println(millis());  
}

void socketOff(int i){
  digitalWrite(dout[i], LOW);
  out[i] = false;
  updateWebPageBody();
  server.send(200, "text/html", webPage);
}

void blinkStatusLED(int high, int low) {
  digitalWrite(doutled, HIGH);
  delay(high);
  digitalWrite(doutled, LOW);
  delay(low);
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


unsigned long getTime() {
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
    Serial.println(timeToString(epoch));
  }
  lastCheckEpoch = epoch;
  return epoch;
}

String timeToString(unsigned long epoch){
    String oss = "UTC: " + String(((epoch  % 86400L) / 3600)) + ":";
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

boolean isHourForTrigger(long epoch, int hour){
  return ((epoch  % 86400L) / 3600) == hour;
}

void checkTimers(){
  long currentTime = epochTime + millis()/1000;
  if (currentTime - lastCheckEpoch > 10){

    if (isHourForTrigger(currentTime, hourOn)){
      Serial.println("Should switchOn lights!");
    }

    if (isHourForTrigger(currentTime, hourOff)){
      Serial.println("Should switchOff lights!");
    }
    Serial.println("Time:" + timeToString(currentTime));
    lastCheckEpoch = currentTime;
  }
}

 
void updateWebPageBody() {
  webPage = "";
  webPage += "<!DOCTYPE HTML>\r\n<html>\n<head>\n";
  webPage += "<meta charset=\"utf-8\"> \n";
  webPage += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
  webPage += "<title>HOME LIGHTS Arduino</title>\n";
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
  webPage += "</head>\n<body>\n";
  webPage += "<h1>Garden Lights v0.21 </h1>\n";
  //Status
  webPage += "<div id=\"accordion\">\n";
  // Controll
  webPage += "<h2>Control: </h2>\n <div>\n";
  for (int i=0; i<sizeof(out); i++){  
    webPage += "<p> ";
    if (out[i]) {
      webPage += " <a href=\"socket"+String(i+1)+"Off\"><button class=\"btn btn-danger btn-lg\">OFF Lights "+String(i+1)+" </button></a>\n";
    } else {
      webPage += " <a href=\"socket"+String(i+1)+"On\"> <button class=\"btn btn-success btn-lg\">ON Lights "+String(i+1)+" </button></a>\n ";
    }
    webPage += "</p> \n";
  }
  webPage += "<p> <a href=\"allOn\">  <button class=\"btn btn-success btn-lg\">ON ALL</button></a> &nbsp; <a href=\"allOff\">  <button class=\"btn btn-danger btn-lg\">OFF ALL</button></a></p> \n";
  webPage += "</div>\n";

  webPage += "<h2>Status: </h2>\n <div>\n";
  webPage += timeToString(epochTime) + "\n" ;
  for (int i=0; i < sizeof(out); i++){  
    if (out[i]) {
      webPage += " <p class=\"bg-success\">Lights "+String(i+1)+" are ON</p>\n";
    } else {
      webPage += " <p class=\"bg-danger\">Lights "+String(i+1)+" are OFF</p>\n";
    }
  }
  webPage += "</div>\n";

  webPage += "<h2>Configuration: </h2>\n<div>\n";
  webPage += "<form  action=\"ScheduleSave\" method=\"get\">\n";
  webPage += "<fieldset>\n";
  webPage += "  <legend>Automatic switch ON/OFF:</legend>\n";

  char* days[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
  for (int i = 1; i <= 7; i++){
    webPage += "  <label for=\"w-"+String(i)+"\">"+ String(days[i-1])+"</label>\n";
    webPage += "  <input type=\"checkbox\" name=\"w-"+String(i)+"\" id=\"w-"+String(i)+"\" " + checked(i)+ ">\n";
  }
  
  webPage += " </fieldset>\n";  
  webPage += " <fieldset>\n";  
  webPage += "  <label for=\"startHour\">Lights ON:</label> \n  <input id=\"startHour\" name=\"startHour\" value=\"";
  webPage += String(hourOn)+"\"> \n";
  webPage += "  <label for=\"endHour\">Lights OFF:</label> \n  <input id=\"endHour\" name=\"endHour\" value=\"";
  webPage += String(hourOff)+"\"> \n";
  webPage += "</fieldset>\n";

  
  webPage += "<fieldset>\n";
  webPage += " <input type=\"submit\" value=\"Save changes\" class=\"btn btn-success btn-lg\"> \n";
  //webPage += "<a href=\"ScheduleSave\"><button type=\"submit\" class=\"btn btn-success btn-lg\">Save changes</button></a>\n";
  webPage += " </fieldset>\n";  
  webPage += " </form>\n";
  
  webPage += " </div>\n";
  webPage += "</div>\n";
  
  //webPage += "<div style=\"font: arial;font-size: 12px;\"><p><iframe src=\"timer.conf\" width=200 height=400 frameborder=0 ></iframe></p></div>\n";  
  webPage += "</body>\n</html>\n";
}

String checked(int day){
  if (daysScheduled.charAt(day) == '1'){
    return "checked";    
  } else {
    return "";
  }
}

