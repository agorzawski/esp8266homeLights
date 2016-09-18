#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include "FS.h"
#include <string>
#include <list>

MDNSResponder mdns;
//local wifi settings
const char* ssid = "a_r_o_2";
const char* password = "Cern0wiec";
//timer variables
unsigned long epochTime;
unsigned long lastCheckEpoch;
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP

// outputs and its configuration
//todo change to some nice lists
int dout[] = {4,5,0};
boolean out[] = {false, false, false};
int hourOn[] = {0,0,0};
int hourOff[] = {1,1,1};
String daysScheduled[] = {"01111110","01111110","00000011"};

int doutled=2;
ESP8266WebServer server(80);
String webPage = "";
void setup(void) {
  updateWebPageBody();

  // preparing GPIOs

  for (int i=0; i< sizeof(dout); i++){
    pinMode(dout[i], OUTPUT);
    digitalWrite(dout[i], LOW);
  }
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
    f.println("#1");  
    f.println("00000011");
    f.println("18");
    f.println("23");
    f.println("#2");   
    f.println("01111111");
    f.println("20");
    f.println("2");
    f.println("#3");    
    f.println("01100111");
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
    int lineCount=-1;
    int channelIndex=-1;
    while(f.available()) {
      //Lets read line by line from the file
      String line = f.readStringUntil('\n');
      Serial.println(line);      
      lineCount++;
      if (lineCount % 4 == 0 ){
              channelIndex++;
              lineCount=0;
              continue;
      }
      switch (lineCount){
        case 1 : daysScheduled[channelIndex] = line; break;
        case 2 : hourOn[channelIndex] = line.toInt(); break;
        case 3 : hourOff[channelIndex] = line.toInt(); break;
      }
    }
    Serial.println("Configuration READ!");
  }
  f.close();
  SPIFFS.end();
  // Set HTML server responses
  server.on("/", []() {
    server.send(200, "text/html", webPage);
  });

  server.on("/ScheduleSave", HTTP_GET,  [](){
    String days[] = {"w-1","w-2","w-3","w-4","w-5","w-6","w-7"};
    for (int ch = 0; ch < sizeof(out); ch++){    
      String hourOn_s=server.arg("ch-"+String(ch)+"-startHour");
      String hourOff_s=server.arg("ch-"+String(ch)+"-endHour");
      Serial.println("GET hourOn: " + hourOn_s);
      hourOn[ch] = hourOn_s.toInt();
      hourOff[ch] = hourOff_s.toInt();    
      String configString = "0";
      for (String one: days){
        String state=server.arg("ch-"+String(ch)+"-"+one);
        if (state == "on"){
          configString += "1";
        } else{
          configString += "0";
        }
      }
      daysScheduled[ch] = configString;  
    }
    //saveFile(daysScheduled, hourOn, hourOff) ;
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
    for (int i = 0; i < sizeof(out); i++){
      digitalWrite(dout[i], HIGH);
      out[i]=true;
    }
    updateWebPageBody();
    server.send(200, "text/html", webPage);
    Serial.println("All ON...");
  });
  server.on("/allOff", []() {
    for (int i = 0; i < sizeof(out); i++){
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
    for (int ch = 0; ch < sizeof(out); ch++){
      if (isHourForTrigger(currentTime, hourOn[ch])){
        Serial.println("Should switchOn lights for Channel:"+String(ch));
      }
  
      if (isHourForTrigger(currentTime, hourOff[ch])){
        Serial.println("Should switchOff lights for Channel:"+String(ch));
      }  
    }
    Serial.println("Time:" + timeToString(currentTime));
    lastCheckEpoch = currentTime;
  }
}

 
void updateWebPageBody() {
  String days[] = {"Mon","Tue","Wed","Thu","Fri","Sat","Sun"};
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
  //jquery
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
  webPage  += "   $( \"#tabs\" ).tabs();\n";
  webPage += "} );\n";
  webPage += "</script>\n";
  webPage += "</head>\n<body>\n";
  webPage += "<h1>Garden Lights v0.21 </h1>\n";

  webPage += "<div id=\"tabs\">\n";
  webPage +=  "<ul> <li><a href=\"#tabs-1\">Control</a></li><li><a href=\"#tabs-2\">Status</a></li>";
  webPage += "  <li><a href=\"#tabs-3\">Config</a></li></ul>";
  // Controll
  webPage += "<div id=\"tabs-1\">\n";
  for (int i=0; i<sizeof(out); i++){  
    webPage += "<p> ";
    if (out[i]) {
      webPage += " <a href=\"socket"+String(i+1)+"Off\"><button class=\"btn btn-danger btn-lg\">OFF Lights "+String(i+1)+" </button></a>";
    } else {
      webPage += " <a href=\"socket"+String(i+1)+"On\"> <button class=\"btn btn-success btn-lg\">ON Lights "+String(i+1)+" </button></a>";
    }
    webPage += "</p> \n";
  }
  webPage += "<p> <a href=\"allOn\">  <button class=\"btn btn-success btn-lg\">ON ALL</button></a> &nbsp; <a href=\"allOff\">  <button class=\"btn btn-danger btn-lg\">OFF ALL</button></a></p> \n";
  webPage += "</div>\n";
  //Status
  webPage += "<div id=\"tabs-2\">\n";
  webPage += timeToString(epochTime) + "\n" ;
  for (int i=0; i < sizeof(out); i++){  
    if (out[i]) {
      webPage += " <p class=\"bg-success\">Lights "+String(i+1)+" are ON</p>\n";
    } else {
      webPage += " <p class=\"bg-danger\">Lights "+String(i+1)+" are OFF</p>\n";
    }
  }
  webPage += "</div>\n";
  //Config
  webPage += "<div id=\"tabs-3\">\n";
  webPage += "<form  action=\"ScheduleSave\" method=\"get\">\n";
  for (int ch = 0; ch < sizeof(out); ch++){
      webPage += "<fieldset>\n";
      webPage += "  <legend>Automatic switch ON/OFF for CHANNEL "+String(ch+1)+":</legend>\n";     
      for (int i = 1; i <= 7; i++){
        webPage += "  <label for=\"ch-"+String(ch)+"-w-"+String(i)+"\">"+ String(days[i-1])+"</label>\n";
        webPage += "  <input type=\"checkbox\" name=\"ch-"+String(ch)+"-w-"+String(i)+"\" id=\"ch-";
        webPage +=String(ch)+"-w-"+String(i)+"\" " + checked(ch,i)+ "/>\n";
      }      
      webPage += " </fieldset>\n";  
      
      webPage += " <fieldset>\n";  
      webPage += "  <label for=\"ch-"+String(ch)+"-startHour\">Lights ON:</label> \n  <input id=\"ch-";
      webPage += String(ch)+"-startHour\" name=\"ch-"+String(ch)+"-startHour\" value=\"";
      webPage += String(hourOn[ch])+"\"/> \n";
      webPage += "  <label for=\"ch-"+String(ch)+"-endHour\">Lights OFF:</label> \n  <input id=\"ch-";
      webPage +=String(ch)+"-endHour\" name=\"ch-"+String(ch)+"-endHour\" value=\"";
      webPage += String(hourOff[ch])+"\"/> \n";
      webPage += "</fieldset>\n";
  }
  
  webPage += "<fieldset>\n";
  webPage += " <input type=\"submit\" value=\"Save changes\" class=\"btn btn-success btn-lg\"/> \n";
  webPage += " </fieldset>\n";  
  webPage += " </form>\n";
  
  webPage += " </div>\n";
  webPage += "</div>\n";
  
  //webPage += "<div style=\"font: arial;font-size: 12px;\"><p><iframe src=\"timer.conf\" width=200 height=400 frameborder=0 ></iframe></p></div>\n";  
  webPage += "</body>\n</html>\n";
}

void saveFile(String days[], int hourOn[], int hourOff[]){
  
}

String checked(int channel, int day){
  if (daysScheduled[channel].charAt(day) == '1'){
    return "checked";    
  } else {
    return "";
  }
}

