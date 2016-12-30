#include "Arduino.h"
#include "TimerOnChannel.h"

TimerOnChannel::TimerOnChannel(int pin, String label)
{
   _pin = pin;
   pinMode(pin, OUTPUT);
   digitalWrite(_pin, LOW);
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

void TimerOnChannel::setOn()
{
  _manual = true;
  digitalWrite(_pin, HIGH);
  _isOn = true;
}

void TimerOnChannel::setOff()
{
  _manual = true;
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
  if (_hourOn == 0 && _hourOn == 0)
  {
    return;  
  }
  if (isForeseenToBeActive(timeInMillis)) // sould be on
  {
    if (!isOn() && !_manual)  // off and auto
    {
          digitalWrite(_pin, HIGH);
          _isOn = true;
    }
  } 
  else // should be off
  {
    if (isOn() && !_manual) // on and auto
    {
        digitalWrite(_pin, LOW);
        _isOn = false;
        // switch off and restore auto mode if after ultimate off time!
        if (getNbOfHours(timeInMillis) >= _hourUltimateOff)
        {
          _manual = false;
        }
    }
  }   
}

boolean TimerOnChannel::isForeseenToBeActive(long timeInMillis)
{
  int nbOfHours = getNbOfHours(timeInMillis);
  if (_manual)
  {
    return (nbOfHours < _hourUltimateOff);
    //TODO for the time being only simple thing like that, add more clever things for after midnight deadline
  } else // in auto mode
  {
    return ((nbOfHours >= _hourOn) &&  (nbOfHours < _hourOff));  
  }
}

int TimerOnChannel::getNbOfHours(long timeInMillis)
{
  return ((timeInMillis  % 86400L) / 3600) + 1; //TODO GMT+1 (GVA)
}

//int TimerOnChannel::getDayNumber(long timeInMillis)
//{
//  return dayOfMonth + ((month < 3) ? (int)((306 * month - 301) / 10) : (int)((306 * month - 913) / 10) + ((year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)) ? 60 : 59));
//}

