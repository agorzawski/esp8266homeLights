#include "FS.h"

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);
  delay(1000);
  Serial.begin(115200);
  blinkStatusLED(150, 150);
  SPIFFS.begin();
  File f = SPIFFS.open("/timer.conf", "w");
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

  Serial.println("Config file created!");
  Serial.println(f.name());

  f.close();
}

void blinkStatusLED(int high, int low) {
  digitalWrite(2, HIGH);
  delay(high);
  digitalWrite(2, LOW);
  delay(low);
}
void loop() {
    blinkStatusLED(50, 500);
}
