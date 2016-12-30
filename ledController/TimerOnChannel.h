#ifndef TimerOnChannel_h
#define TimerOnChannel_h
#include "Arduino.h"

class TimerOnChannel
{
  public:
    TimerOnChannel(int pin, String label);
    void configure(int hourOn, int hourOff, int hourUltimateOff, String dayPattern);   
    
    void setOn(boolean manual);
    void setOff(boolean manual);
    boolean isOn();    
    
    int hourOn();
    int hourOff();
    int hourUltimateOff();

    boolean isManuallyEnabled();
    void restoreAuto();
    
    boolean isForesseenToBeActive(long timeInMillis);
    void adaptStateToConfigurationFor(long timeInMillis);
    
    String getLabel();
    void updateLabel(String label);
    void printStatus();

  private:
    int _pin;
    int _hourOn = -1;
    int _hourOff = -1;
    int _hourUltimateOff = -1;
    boolean _isOn = false;
    String _dayPattern = "11111111";
    String _label = "Lights";
    boolean _manual = false;
    
    static int getNbOfHours(long timeInMillis);
};

#endif
