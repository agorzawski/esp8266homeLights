#ifndef TimerOnChannel_h
#define TimerOnChannel_h
#include "Arduino.h"

class TimerOnChannel
{
  public:
    TimerOnChannel(int pin, String label);
    void configure(int hourOn, int hourOff, String dayPattern);
    
    void setOn();
    void setOff();
    int hourOn();
    int hourOff();
    boolean isOn();
    String getLabel();
    boolean isForesseenToBeActive(long timeInMillis);
    void printStatus();
    
  private:
    int _pin;
    int _hourOn = -1;
    int _hourOff = -1;
    boolean _isOn = false;
    String _dayPattern = "11111111";
    String _label = "Lights";
    static int getNbOfHours(long timeInMillis);
};

#endif
