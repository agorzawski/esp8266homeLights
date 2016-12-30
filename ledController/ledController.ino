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
const char* ssid = "****";
const char* password = "*****";
unsigned long acqEpochTimeFromService;
unsigned long lastCheckTimestamp;
unsigned long currentTime;
TimeService timeService;

//initial values;
int hourOn[] =  {11, 17, 0, 0};
int hourOff[] = {14, 24, 0, 0};
int hourUltimateOff[] = {24, 24, 0, 0};
String daysScheduled[] = {"0000111","01111110","00000011","00000011"};
std::vector<TimerOnChannel> channels;

void setup(void) {
  Serial.println("---=== SYSTEM STARTUP... ===----");
  //D0 - 4, D1 - 5, D3 - 0, D4 - 2 (LED), D5 - 14, D6 - 12, D7 - 13, D8 - 15;
  channels = {TimerOnChannel(4,"Terace LEDs"), TimerOnChannel(5,"Tree lights"), TimerOnChannel(0,"Wall home"), TimerOnChannel(14,"Wall garden")};
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
    updateWebContentAndResponse();
    });
  server.on("/socket1On", []() { setOn(channels[0]); });
  server.on("/socket1Off", []() { setOff(channels[0]); });
  server.on("/socket1AutoOn", []() { setAuto(channels[0]); });
  
  server.on("/socket2On", []() { setOn(channels[1]); });
  server.on("/socket2Off", []() { setOff(channels[1]); });
  server.on("/socket2AutoOn", []() { setAuto(channels[1]); });
  
  server.on("/socket3On", []() { setOn(channels[2]); });
  server.on("/socket3Off", []() { setOff(channels[2]); });   
  server.on("/socket3AutoOn", []() { setAuto(channels[2]); }); 
  
  server.on("/socket4On", []() { setOn(channels[3]); });
  server.on("/socket4Off", []() { setOff(channels[3]); });    
  server.on("/socket4AutoOn", []() { setAuto(channels[3]); });
  
  server.on("/allOn", []() {
    for (TimerOnChannel& channel : channels){
      channel.setOn();
    }
    updateWebContentAndResponse();
  });
  server.on("/allOff", []() {
    for (TimerOnChannel& channel : channels){
      channel.setOff();
    }
    updateWebContentAndResponse();
  });
  server.on("/allAutoOn", []() { 
    for (TimerOnChannel& channel : channels){
      channel.restoreAuto();
    }  
    checkTimers();
    updateWebContentAndResponse();
  });
  server.on("/resetTime", []() {
    startTimeService();
    checkTimers();
    updateWebContentAndResponse();
  });
  server.on("/ScheduleSave", HTTP_GET,  [](){
    String days[] = {"w-1","w-2","w-3","w-4","w-5","w-6","w-7"};
    Serial.println("--= Re-Configuring channels: START =--");
    int index = 0;
    for (TimerOnChannel& channel : channels)
    {   
      String hourOn_s = server.arg("ch-"+String(index)+"-startHour");
      String hourOff_s = server.arg("ch-"+String(index)+"-endHour");
      String hourUltimateOff_s = server.arg("ch-"+String(index)+"-endUltimateHour");
      String label = server.arg("ch-"+String(index)+"-name");
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
      channel.configure(hourOn_s.toInt(), hourOff_s.toInt(),hourUltimateOff_s.toInt(), configString);
      channel.updateLabel(label);
      channel.printStatus();
      index++;
    }
    Serial.println("--= Re-Configuring channles: DONE =---");
    //TODO
    //saveFile(daysScheduled, hourOn, hourOff) ;
    //update Timers objects!
    updateWebContentAndResponse();
  });
  server.begin();
  Serial.println("---=== HTTP server started ===----");
}

void setOn(TimerOnChannel& channel)
{
  channel.setOn();
  updateWebContentAndResponse();
}

void setOff(TimerOnChannel& channel)
{
  channel.setOff();
  updateWebContentAndResponse();
}

void setAuto(TimerOnChannel& channel)
{
  channel.restoreAuto();
  updateWebContentAndResponse();
}

void startTimeService()
{
  Serial.println("---=== TIME SYSTEMS ===----");
  timeService.init();
  acqEpochTimeFromService = timeService.getTime();
  lastCheckTimestamp = acqEpochTimeFromService;
  Serial.println("---=== TIME SYSTEMS UP ===----");
}

void updateWebContentAndResponse()
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
  currentTime = acqEpochTimeFromService + millis()/1000;
  if (currentTime - lastCheckTimestamp > CHECK_INTERVAL_IN_SECONDS)
  {
    for (TimerOnChannel& channel : channels)
    {
     channel.adaptStateToConfigurationFor(currentTime);
     lastCheckTimestamp = currentTime;
    }
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
  webPage += "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/css/bootstrap.min.css\">\n";
              
  webPage += "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.1.1/jquery.min.js\"></script>\n";
  webPage += "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.3.7/js/bootstrap.min.js\"></script>\n"; 

  webPage += "</head>\n<body>\n <div style=\"max-width: 600px;\"> \n";
  
  webPage += "<button class=\"btn btn-default btn-block disabled\"><h1>Garden Lights v0.23</h1></button> \n";
  webPage += "<button type=\"button\" class=\"btn btn-default btn-block disabled \"><small> Last timer check: "+TimeService::timeToString(lastCheckTimestamp, 1)+"</small></button>";  
  
  webPage += "<div class=\"btn-group btn-group-justified\">";
  webPage += "<a href=\"/\" class=\"btn btn-info btn-sm\">Refresh</a>";
  webPage += "<a href=\"resetTime\" class=\"btn btn-default btn-sm\">Re-establish time</a>";
  webPage += "</div>\n";

  webPage += "\n";
  webPage +=  "<ul class=\"nav nav-tabs\">";
  webPage +=  "<li class=\"active\"><a data-toggle=\"tab\" href=\"#tabs-1\">Home</a></li>";
  webPage += "<li><a data-toggle=\"tab\"  href=\"#tabs-2\">Status expert</a></li>";
  webPage += " <li><a data-toggle=\"tab\" href=\"#tabs-3\">Configuration</a></li>\n </ul>\n";
  // Control
  webPage += "<div class=\"tab-content\">\n";
  webPage += "<div id=\"tabs-1\" class=\"tab-pane fade in active\">\n";
  
  int i = 0;
  int countInManual = 0;
  for (TimerOnChannel& channel : channels)
  {  
    webPage += "<p>  <div class=\"btn-group btn-group-justified\">";
    String disabled = "";
    if (channel.hourOn() == 0 && channel.hourOff() == 0 )
    {
      disabled = "disabled";
    }
    if (channel.isOn())
    {
      webPage += " <a href=\"socket"+String(i+1)+"Off\" class=\"btn btn-danger btn-md "+disabled+"\"> <span class=\"badge\">OFF</span> "+channel.getLabel()+" </a>";
    } else {
      webPage += " <a href=\"socket"+String(i+1)+"On\" class=\"btn btn-success btn-md "+disabled+"\"><span class=\"badge\">ON "+disabled+"</span> "+channel.getLabel()+" </a>";
    }
  
    if (channel.isManuallyEnabled())
    {
      countInManual++;
      webPage += " <a href=\"socket"+String(i+1)+"AutoOn\" class=\"btn btn-warning btn-md\">Restore automatic</a>";
    } else {
      webPage += " <a href=\"\" class=\"btn btn-info btn-md disabled\">Automatic, scheduled for";
      if (channel.isForeseenToBeActive(acqEpochTimeFromService + millis()/1000))
      {
        webPage += " <span class=\"badge\"> ON </span>  </a>";
      } else {
        webPage += " <span class=\"badge\"> OFF </span>  </a>";
      }
    }

    webPage += "</div></p> \n";
    i++;
  }
  webPage += "<div class=\"btn-group btn-group-justified\"> <a href=\"allOn\" class=\"btn btn-success btn-lg\"><span class=\"badge\">ON ALL</span></a>";
  webPage += "<a href=\"allOff\" class=\"btn btn-danger btn-lg\"><span class=\"badge\">OFF ALL</span></a>";
  if (countInManual > 1)
  {
    webPage += "<a href=\"allAutoOn\" class=\"btn btn-warning btn-lg\">AUTO ALL <span class=\"badge\">"+String(countInManual)+"</span> </a>";
  }
  webPage +="</div>\n";
  webPage += "</div>\n";
//  Status
  webPage += "<div id=\"tabs-2\" class=\"tab-pane fade in\"><h3>Detailed status</h3>\n";
  webPage += "<button type=\"button\" class=\"btn btn-default btn-block disabled \"><small> System start: "+TimeService::timeToString(acqEpochTimeFromService, 1)+"</small></button>";  
  webPage += "<button type=\"button\" class=\"btn btn-default btn-block disabled \"><small> Last timer check: "+TimeService::timeToString(lastCheckTimestamp, 1)+"</small></button>";  
  webPage += "<pre> " + timeService.getLastLog() + " </pre>\n";

  long  fh = ESP.getFreeHeap();
  char  fhc[20];
  ltoa(fh, fhc, 10);
  String freeHeap = String(fhc);
  webPage += "<h3>Free memory</h3><samp>"+freeHeap+" kB</samp>";

  webPage += "<h3>Inputs</h3>";
  i = 0;
  for (TimerOnChannel& channel : channels)
  {  
    if (channel.isOn()) 
    {
      webPage += " <p class=\"bg-success\">"+channel.getLabel()+" are ON</p>\n";
    } else {
      webPage += " <p class=\"bg-danger\">"+channel.getLabel()+" are OFF</p>\n";
    }
    i++;
  }
  webPage += "</div>\n";
  //Config
  webPage += "<div id=\"tabs-3\" class=\"tab-pane fade in\"><h3>Times and days for channels</h3>\n";
  // form breaks the bootstrap/jquery style 
  webPage += "<form  action=\"ScheduleSave\" method=\"get\">\n";
  webPage += "  <fieldset>\n";
  webPage += "   <input type=\"submit\" value=\"Save changes\" class=\"btn btn-success btn-block\"/> \n";
  webPage += "  </fieldset>\n";
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
      webPage += String(ch)+"-name\" name=\"ch-"+String(ch)+"-name\" value=\"" +channel.getLabel()+ "\"/><br> \n";
      
      webPage += "  <label for=\"ch-"+String(ch)+"-startHour\">Lights ON:</label> \n  <input id=\"ch-";
      webPage += String(ch)+"-startHour\" name=\"ch-"+String(ch)+"-startHour\" value=\"";
      webPage += String(channel.hourOn())+"\"/><br> \n";
      
      webPage += "  <label for=\"ch-"+String(ch)+"-endHour\">Lights OFF:</label> \n  <input id=\"ch-";
      webPage +=String(ch)+"-endHour\" name=\"ch-"+String(ch)+"-endHour\" value=\"";
      webPage += String(channel.hourOff())+"\"/><br> \n";

      webPage += "  <label for=\"ch-"+String(ch)+"-endUltimateHour\">Lights ALWAYS OFF:</label> \n  <input id=\"ch-";
      webPage +=String(ch)+"-endUltimateHour\" name=\"ch-"+String(ch)+"-endUltimateHour\" value=\"";
      webPage += String(channel.hourUltimateOff())+"\"/><br> \n";
      
      webPage += "</fieldset>\n";
      ch++;
  }  
  
  webPage += " </form>\n";
  webPage += " </div>\n";
  webPage += "</div>\n";
  
  //webPage += "<div style=\"font: arial;font-size: 12px;\"><p><iframe src=\"timer.conf\" width=200 height=400 frameborder=0 ></iframe></p></div>\n";  
  webPage += "</div>\n</body>\n</html>\n";
}

void saveFile(std::vector<TimerOnChannel> channels){
  // TODO
}

String checked(int channel, int day){
  if (daysScheduled[channel].charAt(day) == '1')
  {
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
    Serial.println("--== Channel ("+channel.getLabel()+") index: "+ String(index) + " ==--");
    channel.configure(hourOn[index], hourOff[index], hourUltimateOff[index], daysScheduled[index]);
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

