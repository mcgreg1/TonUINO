#ifndef Modifier_h
#define Modifier_h

//#include "TonUINO.h"
//#include "nfc.h"
//#include "utilities.h"
#include "Arduino.h"

struct folderSettings 
{
  uint8_t folder;
  uint8_t mode;
  uint8_t special;
  uint8_t special2;
};

// this object stores nfc tag data
struct nfcTagObject 
{
  uint32_t cookie;
  uint8_t version;
  folderSettings nfcFolderSettings;

};



class Modifier 
{
  public:
    Modifier();
    virtual void loop();
    virtual bool handlePause();
    virtual bool handleNext(); 
    virtual bool handlePrevious();
    virtual bool handleNextButton();
    virtual bool handlePreviousButton();
    virtual bool handleVolumeUp(); 
    virtual bool handleVolumeDown();
    virtual bool handlePotiVolume(); 
    virtual bool handleRFID(nfcTagObject *newCard); 
    virtual uint8_t getActive(); 

};

extern Modifier *activeModifier;

class SleepTimer: public Modifier {
  private:
    unsigned long sleepAtMillis;

  public:
    void loop(); 

    SleepTimer(uint8_t minutes);
    uint8_t getActive();

};


class Locked: public Modifier {
  public:
    virtual bool handlePause();   
    virtual bool handleNextButton();   
    virtual bool handlePreviousButton(); 
    virtual bool handleVolumeUp();  
    virtual bool handleVolumeDown(); 
    virtual bool handlePotiVolume(); 
    virtual bool handleRFID(nfcTagObject *newCard); 
    Locked(); 
    uint8_t getActive(); 

};

class ToddlerMode: public Modifier {
  public:
    virtual bool handlePause(); 
    virtual bool handleNextButton();  
    virtual bool handlePreviousButton();
    virtual bool handleVolumeUp();  
    virtual bool handleVolumeDown(); 
    virtual bool handlePotiVolume(); 
    ToddlerMode();
    uint8_t getActive(); 
};

class KindergardenMode: public Modifier {
  private:
    nfcTagObject nextCard;
    bool cardQueued;

  public:
    virtual bool handleNext();
    virtual bool handleNextButton();
    virtual bool handlePreviousButton(); 
    virtual bool handleRFID(nfcTagObject * newCard); 
    KindergardenMode();
    uint8_t getActive();
};

class RepeatSingleModifier: public Modifier {
  public:
    virtual bool handleNext(); 
    RepeatSingleModifier();
    uint8_t getActive();
};

// An modifier can also do somethings in addition to the modified action
// by returning false (not handled) at the end
// This simple FeedbackModifier will tell the volume before changing it and
// give some feedback once a RFID card is detected.
class FeedbackModifier: public Modifier {
  public:
    virtual bool handleVolumeDown();
    virtual bool handleVolumeUp();
    virtual bool handleRFID(nfcTagObject * newCard);
    uint8_t getActive(); 
};


class FreezeDance: public Modifier {
  public:

    FreezeDance(); 
    void loop() ;
    uint8_t FreezeDance::getActive(); 
    void FreezeDance::setNextStopAtMillis();
    private: 
  private:
    unsigned long nextStopAtMillis = 0;
    const uint8_t minSecondsBetweenStops = 5;
    const uint8_t maxSecondsBetweenStops = 30;
};


#endif modifier_h
