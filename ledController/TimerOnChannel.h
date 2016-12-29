#ifndef TimerOnChannel_h
#define TimerOnChannel_h
#include "Arduino.h"

class TimerOnChannel
{
  public:
    TimerOnChannel(int pin);
    
    void configure(int hourOn, int hourOff, String dayPattern);
    boolean isForesseenToBeActive(long timeInMillis);
    
  private:
    int _hourOn = -1;
    int _hourOff = -1;
    String _dayPattern = "11111111";
    int _pin;
};

#endif
