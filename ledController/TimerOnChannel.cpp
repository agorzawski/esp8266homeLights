#include "Arduino.h"
#include "TimerOnChannel.h"

TimerOnChannel::TimerOnChannel(int pin, String label)
{
   _pin = pin;
   pinMode(pin, OUTPUT);
   setOff();
   _label = label;
}

String TimerOnChannel::getLabel()
{
  return _label;
}

void TimerOnChannel::setOn()
{
  digitalWrite(_pin, HIGH);
  _isOn = true;
}

void TimerOnChannel::setOff()
{
  digitalWrite(_pin, LOW);
  _isOn = false;
}

int  TimerOnChannel::hourOn()
{
  return _hourOn;
}

int  TimerOnChannel::hourOff()
{
  return _hourOff;
}

boolean TimerOnChannel::isOn()
{
  return _isOn;
}

void TimerOnChannel::printStatus()
{
  Serial.println("--== Pin: "+ String(_pin) + " ==--");
  Serial.println("  ON : "+ String(_hourOn));
  Serial.println(" OFF : "+ String(_hourOff));
  Serial.println("DAYS : "+ _dayPattern);
}

void TimerOnChannel::configure(int hourOn, int hourOff, String dayPattern) 
{
  _hourOn = hourOn;
  _hourOff = hourOff;
  _dayPattern = dayPattern;
}

boolean TimerOnChannel::isForesseenToBeActive(long timeInMillis)
{
  int nbOfHours = getNbOfHours(timeInMillis);
  return ( (nbOfHours >= _hourOn) &&  (nbOfHours <= _hourOff));
}

int TimerOnChannel::getNbOfHours(long timeInMillis)
{
  return ((timeInMillis  % 86400L) / 3600);
}


