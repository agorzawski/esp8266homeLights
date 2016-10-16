#include "Arduino.h"
#include "TimerOnChannel.h"

TimerOnChannel::TimerOnChannel(int pin)
{
  _pin = pin;
   pinMode(pin, OUTPUT);
   digitalWrite(pin, LOW);
}

void TimerOnChannel::configure(int hourOn, int hourOff, String dayPattern) 
{
  _hourOn = hourOn;
  _hourOff = hourOff;
  _dayPattern = dayPattern;
}

boolean TimerOnChannel::isForesseenToBeActive(long timeInMillis)
{
  
  return false;
}

