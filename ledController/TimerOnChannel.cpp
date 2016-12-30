#include "Arduino.h"
#include "TimerOnChannel.h"

TimerOnChannel::TimerOnChannel(int pin, String label)
{
   _pin = pin;
   pinMode(pin, OUTPUT);
   setOff(false);
   _label = label;
}

String TimerOnChannel::getLabel()
{
  return _label;
}

void TimerOnChannel::updateLabel(String label)
{
  _label = label;
}

void TimerOnChannel::setOn(boolean manual)
{
    _manual = manual;
  digitalWrite(_pin, HIGH);
  _isOn = true;
}

void TimerOnChannel::setOff(boolean manual)
{
  _manual = manual;
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

int  TimerOnChannel::hourUltimateOff()
{
  return _hourUltimateOff;
}


boolean TimerOnChannel::isOn()
{
  return _isOn;
}

boolean TimerOnChannel::isManuallyEnabled()
{
  return _manual;
}

void TimerOnChannel::restoreAuto()
{
  _manual = false;
}

void TimerOnChannel::printStatus()
{
  Serial.println("--== Pin: "+ String(_pin) + " ==--");
  Serial.println("  ON : "+ String(_hourOn));
  Serial.println(" OFF : "+ String(_hourOff));
  Serial.println("DAYS : "+ _dayPattern);
}

void TimerOnChannel::configure(int hourOn, int hourOff, int hourUltimateOff, String dayPattern) 
{
  _hourOn = hourOn;
  _hourOff = hourOff;
  _hourUltimateOff = hourUltimateOff;
  _dayPattern = dayPattern;
}

void TimerOnChannel::adaptStateToConfigurationFor(long timeInMillis)
{
  if (isForesseenToBeActive(timeInMillis))
  {
    if (!isOn() && !_manual)
      {
          digitalWrite(_pin, HIGH);
          _isOn = true;
      }
  } 
  else 
  {
    if (isOn()) 
      {
        digitalWrite(_pin, LOW);
        _isOn = false;
        //Switch off and restore auto mode if after ultimate off!
        if (getNbOfHours(timeInMillis) >= _hourUltimateOff){
          _manual = false;
        }
      }
  }   
}

boolean TimerOnChannel::isForesseenToBeActive(long timeInMillis)
{
  int nbOfHours = getNbOfHours(timeInMillis);
  if (_manual)
  {
    return (nbOfHours < _hourUltimateOff);
  } else 
  {
    return ((nbOfHours >= _hourOn) &&  (nbOfHours < _hourOff));  
  }
}

int TimerOnChannel::getNbOfHours(long timeInMillis)
{
  return ((timeInMillis  % 86400L) / 3600) + 1; //TODO GMT+1 (GVA)
}


