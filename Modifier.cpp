#include "Modifier.h"
#include "TonUINO.h"


Modifier::Modifier() {}
void Modifier::loop() {}
bool Modifier::handlePause() {
  return false;
}
bool Modifier::handleNext() {
  return false;
}
bool Modifier::handlePrevious() {
  return false;
}
bool Modifier::handleNextButton() {
  return false;
}
bool Modifier::handlePreviousButton() {
  return false;
}
bool Modifier::handleVolumeUp() {
  return false;
}
bool Modifier::handleVolumeDown() {
  return false;
}
bool Modifier::handlePotiVolume() {
  return false;
}
bool Modifier::handleRFID(nfcTagObject *newCard) {
  return false;
}
uint8_t Modifier::getActive() {
  return 0;
}



SleepTimer::SleepTimer(uint8_t minutes) 
{
      Serial.println(F("=== SleepTimer()"));
      Serial.println(minutes);
      this->sleepAtMillis = millis() + minutes * 60000;
      //      if (isPlaying())
      //        mp3.playAdvertisement(302);
      //      delay(500);
}



void SleepTimer::loop() 
{
  if (this->sleepAtMillis != 0 && millis() > this->sleepAtMillis) 
  {
    Serial.println(F("=== SleepTimer::loop() -> SLEEP!"));
    mp3.pause();
    setstandbyTimer();
    activeModifier = NULL;
    delete this;
  }
}

    
uint8_t SleepTimer::getActive() 
{
    Serial.println(F("== SleepTimer::getActive()"));
    return 1;
}



Locked::Locked() 
{
  Serial.println(F("=== Locked()"));
}
bool Locked::handlePause()     
{
  Serial.println(F("== Locked::handlePause() -> LOCKED!"));
  return true;
}
bool Locked::handleNextButton()       
{
  Serial.println(F("== Locked::handleNextButton() -> LOCKED!"));
  return true;
}
bool Locked::handlePreviousButton() 
{
  Serial.println(F("== Locked::handlePreviousButton() -> LOCKED!"));
  return true;
}
bool Locked::handleVolumeUp()   
{
  Serial.println(F("== Locked::handleVolumeUp() -> LOCKED!"));
  return true;
}
bool Locked::handleVolumeDown() {
  Serial.println(F("== Locked::handleVolumeDown() -> LOCKED!"));
  return true;
}
bool Locked::handlePotiVolume() 
{
  Serial.println(F("== Locked::handlePotiVolume() -> LOCKED!"));
  return true;
}
bool Locked::handleRFID(nfcTagObject *newCard) 
{
  Serial.println(F("== Locked::handleRFID() -> LOCKED!"));
  return true;
}

uint8_t Locked::getActive() 
{
  return 3;
}


ToddlerMode::ToddlerMode() 
{
  Serial.println(F("=== ToddlerMode()"));
  //      if (isPlaying())
  //        mp3.playAdvertisement(304);
}
bool ToddlerMode::handlePause()     
{
  Serial.println(F("== ToddlerMode::handlePause() -> LOCKED!"));
  return true;
}
bool ToddlerMode::handleNextButton()       
{
  Serial.println(F("== ToddlerMode::handleNextButton() -> LOCKED!"));
  return true;
}
bool ToddlerMode::handlePreviousButton() 
{
  Serial.println(F("== ToddlerMode::handlePreviousButton() -> LOCKED!"));
  return true;
}
bool ToddlerMode::handleVolumeUp()   
{
  Serial.println(F("== ToddlerMode::handleVolumeUp() -> LOCKED!"));
  return true;
}
bool ToddlerMode::handleVolumeDown() 
{
  Serial.println(F("== ToddlerMode::handleVolumeDown() -> LOCKED!"));
  return true;
}
bool ToddlerMode::handlePotiVolume() 
{
  Serial.println(F("== ToddlerMode::handlePotiVolume() -> LOCKED!"));
  return true;
}

uint8_t ToddlerMode::getActive() 
{
  Serial.println(F("== ToddlerMode::getActive()"));
  return 4;
}


KindergardenMode::KindergardenMode() 
{
  Serial.println(F("=== KindergardenMode()"));
  //      if (isPlaying())
  //        mp3.playAdvertisement(305);
  //      delay(500);
}


bool KindergardenMode::handleNext() 
{
  Serial.println(F("== KindergardenMode::handleNext() -> NEXT"));
  //if (this->nextCard.cookie == cardCookie && this->nextCard.nfcFolderSettings.folder != 0 && this->nextCard.nfcFolderSettings.mode != 0) {
  //myFolder = &this->nextCard.nfcFolderSettings;
  if (this->cardQueued == true) {
    this->cardQueued = false;

    myCard = nextCard;
    myFolder = &myCard.nfcFolderSettings;
    Serial.println(myFolder->folder);
    Serial.println(myFolder->mode);
    playFolder();
    return true;
  }
  return false;
}

bool KindergardenMode::handleNextButton()       
{
  Serial.println(F("== KindergardenMode::handleNextButton() -> LOCKED!"));
  return true;
}

bool KindergardenMode::handlePreviousButton() 
{
  Serial.println(F("== KindergardenMode::handlePreviousButton() -> LOCKED!"));
  return true;
}

bool KindergardenMode::handleRFID(nfcTagObject * newCard) 
{ // lot of work to do!
  Serial.println(F("== KindergardenMode::handleRFID() -> queued!"));
  this->nextCard = *newCard;
  this->cardQueued = true;
  if (!isPlaying()) 
  {
    handleNext();
  }
  return true;
}

uint8_t KindergardenMode::getActive() 
{
  Serial.println(F("== KindergardenMode::getActive()"));
  return 5;
}

RepeatSingleModifier::RepeatSingleModifier()
{
  Serial.println(F("=== RepeatSingleModifier()"));
   
}
bool RepeatSingleModifier::handleNext() 
{
  Serial.println(F("== RepeatSingleModifier::handleNext() -> REPEAT CURRENT TRACK"));
  delay(50);
  if (isPlaying()) return true;
  mp3.playFolderTrack(myFolder->folder, currentTrack);
  _lastTrackFinished = 0;
  return true;
}
      
uint8_t RepeatSingleModifier::getActive() 
{
  Serial.println(F("== RepeatSingleModifier::getActive()"));
  return 6;
}


// An modifier can also do somethings in addition to the modified action
// by returning false (not handled) at the end
// This simple FeedbackModifier will tell the volume before changing it and
// give some feedback once a RFID card is detected.
//FeedbackModifier::FeedbackModifier(){};

bool FeedbackModifier::handleVolumeDown() 
{
  if (volume > mySettings.minVolume) {
    mp3.playAdvertisement(volume - 1);
  }
  else {
    mp3.playAdvertisement(volume);
  }
  delay(500);
  Serial.println(F("== FeedbackModifier::handleVolumeDown()!"));
  return false;
}
bool FeedbackModifier::handleVolumeUp() 
{
  if (volume < mySettings.maxVolume) {
    mp3.playAdvertisement(volume + 1);
  }
  else {
    mp3.playAdvertisement(volume);
  }
  delay(500);
  Serial.println(F("== FeedbackModifier::handleVolumeUp()!"));
  return false;
}
bool FeedbackModifier::handleRFID(nfcTagObject *newCard) 
{
  Serial.println(F("== FeedbackModifier::handleRFID()"));
  return false;
}



FreezeDance::FreezeDance() 
{
  Serial.println(F("=== FreezeDance()"));
  if (isPlaying()) {
    delay(1000);
    mp3.playAdvertisement(300);
    delay(500);
  }
  setNextStopAtMillis();
}

void FreezeDance::setNextStopAtMillis() 
{
  uint16_t seconds = random(this->minSecondsBetweenStops, this->maxSecondsBetweenStops + 1);
  Serial.println(F("=== FreezeDance::setNextStopAtMillis()"));
  Serial.println(seconds);
  this->nextStopAtMillis = millis() + seconds * 1000;
}

void FreezeDance::loop() 
{
  if (this->nextStopAtMillis != 0 && millis() > this->nextStopAtMillis) {
    Serial.println(F("== FreezeDance::loop() -> FREEZE!"));
    if (isPlaying()) {
      mp3.playAdvertisement(301);
      delay(500);
    }
    setNextStopAtMillis();
  }
}

uint8_t FreezeDance::getActive() 
{
  Serial.println(F("== FreezeDance::getActive()"));
  return 2;
}
