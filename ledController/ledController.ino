#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <vector>

#include "TimerOnChannel.h"
#include "TimeService.h"

#define LED_PIN 2
#define CHECK_INTERVAL_IN_SECONDS 15

MDNSResponder mdns;
ESP8266WebServer server(80);
String webPage = "";
//local wifi settings
const char* ssid = "a_r_o_2";
const char* password = "Cern0wiec";
unsigned long epochTime;
unsigned long lastCheckEpoch;

//initial values;
int hourOn[] =  {16, 17, -1, -1};
int hourOff[] = {17, 23, -1, -1};
String daysScheduled[] = {"01111110","01111110","00000011","00000011"};
std::vector<TimerOnChannel> channels;

void setup(void) {
  Serial.println("---=== SYSTEM STARTUP... ===----");
  //D0 - 4, D1 - 5, D3 - 0, D4 - 2 (LED), D5 - 14, D6 - 12, D7 - 13, D8 - 15;
  channels = {TimerOnChannel(4,"Terace LED"), TimerOnChannel(5,"Tree lights"), TimerOnChannel(0,"Wall home"), TimerOnChannel(14,"Wall garden")};
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(1000);
  Serial.begin(115200);
  connectWifi();
  configureChannels();
  startHttpService(); 
  startTimeService();
  Serial.println("---=== ALL SYSTEMS UP ===----");
}

void loop(void) 
{
  blinkStatusLED(1000, 1000);
  checkTimers();
  server.handleClient();
}

void connectWifi()
{  
  // Begin connection
  WiFi.begin(ssid, password);
  Serial.println("WIFI: ");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    blinkStatusLED(250, 250);
    Serial.print(".");
  }
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
}

void startHttpService()
{
  Serial.println("---=== HTTP server starting... ===----");
  server.on("/", []() {      
    updateContentAndResponses();
    });
  server.on("/socket1On", []() {
    channels.at(0).setOn();
    updateContentAndResponses();
    });
  server.on("/socket1Off", []() {
    channels.at(0).setOff();
    updateContentAndResponses();
    });
  server.on("/socket2On", []() {
    channels.at(1).setOn();
    updateContentAndResponses();
    });
  server.on("/socket2Off", []() {
    channels.at(1).setOff();
    updateContentAndResponses();
    });
  server.on("/socket3On", []() {
    channels.at(2).setOn();
    updateContentAndResponses();
    });
  server.on("/socket3Off", []() {
    channels.at(2).setOff();
    updateContentAndResponses();
    });    
  server.on("/allOn", []() {
    for (TimerOnChannel& channel : channels){
      channel.setOn();
    }
    updateContentAndResponses();
  });
  server.on("/allOff", []() {
    for (TimerOnChannel& channel : channels){
      channel.setOff();
    }
    updateContentAndResponses();
  });
  server.on("/resetTime", []() {
    startTimeService();
    updateContentAndResponses();
  });
  server.on("/ScheduleSave", HTTP_GET,  [](){
    String days[] = {"w-1","w-2","w-3","w-4","w-5","w-6","w-7"};
    Serial.println("--= Re-Configuring channels: START =--");
    int index = 0;
    for (TimerOnChannel& channel : channels)
    {   
      String hourOn_s = server.arg("ch-"+String(index)+"-startHour");
      String hourOff_s = server.arg("ch-"+String(index)+"-endHour");
      Serial.println("GET hourOn: " + hourOn_s);
      String configString = "0";
      for (String one: days){
        String state=server.arg("ch-"+String(index)+"-"+one);
        if (state == "on"){
          configString += "1";
        } else{
          configString += "0";
        }
      }
      Serial.println("-- Channel ("+channel.getLabel()+") internal index: "+ String(index) + " --");
      channel.configure(hourOn_s.toInt(), hourOff_s.toInt(), configString);
      channel.printStatus();
      index++;
    }
    Serial.println("--= Re-Configuring channles: DONE =---");
    //TODO
    //saveFile(daysScheduled, hourOn, hourOff) ;
    //update Timers objects!
    updateContentAndResponses();
  });
  server.begin();
  Serial.println("---=== HTTP server started ===----");
}

void startTimeService()
{
  Serial.println("---=== TIME SYSTEMS ===----");
  TimeService timeService;
  epochTime = timeService.getTime();
  lastCheckEpoch = epochTime;
  Serial.println("---=== TIME SYSTEMS UP ===----");
}

void updateContentAndResponses()
{
   updateWebPageBody();
   server.send(200, "text/html", webPage);
}

void blinkStatusLED(int high, int low) 
{
  digitalWrite(LED_PIN, HIGH);
  delay(high);
  digitalWrite(LED_PIN, LOW);
  delay(low);
}

void checkTimers()
{
  long currentTime = epochTime + millis()/1000;
  if (currentTime - lastCheckEpoch > CHECK_INTERVAL_IN_SECONDS){
    int index = 0;
    for (TimerOnChannel& channel : channels){
      if (channel.isForesseenToBeActive(currentTime))
      {
        if (!channel.isOn()) channel.setOn();
      } else 
      {
        if (channel.isOn()) channel.setOff();
      }
      index++;
    }
    lastCheckEpoch = currentTime;
  }
}

 
void updateWebPageBody() 
{
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
  
  int i = 0;
  for (TimerOnChannel& channel : channels)
  {  
    webPage += "<p> ";
    if (channel.isOn())
    {
      webPage += " <a href=\"socket"+String(i+1)+"Off\"><button class=\"btn btn-danger btn-lg\">OFF Lights "+channel.getLabel()+" </button></a>";
    } else {
      webPage += " <a href=\"socket"+String(i+1)+"On\"> <button class=\"btn btn-success btn-lg\">ON Lights "+channel.getLabel()+" </button></a>";
    }
    webPage += "</p> \n";
    i++;
  }
  webPage += "<p> <a href=\"allOn\">  <button class=\"btn btn-success btn-lg\">ON ALL</button></a> &nbsp; <a href=\"allOff\">  <button class=\"btn btn-danger btn-lg\">OFF ALL</button></a></p> \n";
  webPage += "</div>\n";
  //Status
  webPage += "<div id=\"tabs-2\">\n";
  webPage += TimeService::timeToString(epochTime) + "\n" ;
  i = 0;
  for (TimerOnChannel& channel : channels)
  {  
    if (channel.isOn()) 
    {
      webPage += " <p class=\"bg-success\">Lights "+channel.getLabel()+" are ON</p>\n";
    } else {
      webPage += " <p class=\"bg-danger\">Lights "+channel.getLabel()+" are OFF</p>\n";
    }
    i++;
  }
  webPage += "</div>\n";
  //Config
  webPage += "<div id=\"tabs-3\">\n";
  // form breaks the bootstrap/jquery style 
  webPage += "<form  action=\"ScheduleSave\" method=\"get\">\n";
  int ch = 0;
  for (TimerOnChannel& channel : channels){
      webPage += "<fieldset>\n";
      webPage += "  <legend>Automatic switch ON/OFF for CHANNEL "+String(ch+1)+":</legend>\n";     
      for (int i = 1; i <= 7; i++){
        webPage += "  <label for=\"ch-"+String(ch)+"-w-"+String(i)+"\">"+ String(days[i-1])+"</label>\n";
        webPage += "  <input type=\"checkbox\" name=\"ch-"+String(ch)+"-w-"+String(i)+"\" id=\"ch-";
        webPage +=String(ch)+"-w-"+String(i)+"\" " + checked(ch,i)+ "/>\n";
      }      
      webPage += " </fieldset>\n";  
      
      webPage += " <fieldset>\n";  
      
      webPage += "  <label>Channel name:</label> \n  <input id=\"ch-";
      webPage += String(ch)+"-name\" name=\"ch-"+String(ch)+"-name\" value=\"" +channel.getLabel()+ "\"/> \n";
      
      webPage += "  <label for=\"ch-"+String(ch)+"-startHour\">Lights ON:</label> \n  <input id=\"ch-";
      webPage += String(ch)+"-startHour\" name=\"ch-"+String(ch)+"-startHour\" value=\"";
      webPage += String(channel.hourOn())+"\"/> \n";
      webPage += "  <label for=\"ch-"+String(ch)+"-endHour\">Lights OFF:</label> \n  <input id=\"ch-";
      webPage +=String(ch)+"-endHour\" name=\"ch-"+String(ch)+"-endHour\" value=\"";
      webPage += String(channel.hourOff())+"\"/> \n";
      webPage += "</fieldset>\n";
      ch++;
  }
  
  webPage += "  <fieldset>\n";
  webPage += "   <input type=\"submit\" value=\"Save changes\" class=\"btn btn-success btn-lg\"/> \n";
  webPage += "  </fieldset>\n";  
  webPage += " </form>\n";
  webPage += " <p> <a href=\"resetTime\"> <button class=\"btn btn-success btn-lg\">Re-establish time</button></a></p> \n";
  webPage += " </div>\n";
  webPage += "</div>\n";
  
  //webPage += "<div style=\"font: arial;font-size: 12px;\"><p><iframe src=\"timer.conf\" width=200 height=400 frameborder=0 ></iframe></p></div>\n";  
  webPage += "</body>\n</html>\n";
}

void saveFile(std::vector<TimerOnChannel> channels){
  // TODO
}

String checked(int channel, int day){
  if (daysScheduled[channel].charAt(day) == '1'){
    return "checked";    
  } else {
    return "";
  }
}

void configureChannels()
{
  openConfiguration();
  Serial.println("---=== Configuring channels: START ===----");
  int index = 0;
  for (TimerOnChannel& channel : channels){
    Serial.println("--== Channel index: "+ String(index) + " ==--");
    channel.configure(hourOn[index], hourOff[index], daysScheduled[index]);
    channel.printStatus();
    index++;
  }
  Serial.println("---=== Configuring channles: DONE ===----");
}

void openConfiguration()
{
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
}

