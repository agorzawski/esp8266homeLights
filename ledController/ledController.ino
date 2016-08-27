#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

MDNSResponder mdns;

const char* ssid = "a_r_o_2";
const char* password = "Cern0wiec";

ESP8266WebServer server(80);

String webPage = "";

int dout1 = 4;
int dout2 = 5;
int doutled = 2;

void setup(void){
  webPage += "<h1>Oswietlenie ogrod: </h1>";
  webPage += "<p>Swiatla klomb <a href=\"socket1On\"><button>ON</button></a>&nbsp;<a href=\"socket1Off\"><button>OFF</button></a></p>";
  webPage += "<p>Swiatla taras <a href=\"socket2On\"><button>ON</button></a>&nbsp;<a href=\"socket2Off\"><button>OFF</button></a></p>";
  webPage += "<p>Wszystkie     <a href=\"allOn\">    <button>ON</button></a>&nbsp;<a href=\"allOff\">    <button>OFF</button></a></p>";
  
  // preparing GPIOs
  pinMode(dout1, OUTPUT);
  digitalWrite(dout1, LOW);
  pinMode(dout2, OUTPUT);
  digitalWrite(dout2, LOW);
  pinMode(doutled, OUTPUT);
  digitalWrite(doutled, LOW);

  uint8_t macAddress[6];
  WiFi.macAddress(macAddress);
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
    delay(500);
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
    server.send(200, "text/html", webPage);
    digitalWrite(dout1, HIGH);
    delay(1000);
    blinkStatusLED(200,200);
  });
  server.on("/socket1Off", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(dout1, LOW);
    delay(1000); 
  });
  server.on("/socket2On", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(dout2, HIGH);
    delay(1000);
    blinkStatusLED(200,200);
  });
  server.on("/socket2Off", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(dout2, LOW);
    delay(1000); 
  });

  server.on("/allOn", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(dout1, HIGH);
    digitalWrite(dout2, HIGH);
    Serial.println("All ON...");
    delay(1000);
  });

   server.on("/allOff", [](){
    server.send(200, "text/html", webPage);
    digitalWrite(dout1, LOW);
    digitalWrite(dout2, LOW);
    Serial.println("All OFF...");
    delay(1000);
  });
  
  server.begin();
  Serial.println("HTTP server started");
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
