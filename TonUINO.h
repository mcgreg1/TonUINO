#ifndef TonUINO_h
#define TonUINO_h

#include "Modifier.h"
#include "nfc.h"
#include "player.h"
#include "utilities.h"

#include <DFMiniMp3.h>
#include <EEPROM.h>
#include <JC_Button.h>

#include <SPI.h>
#include <SoftwareSerial.h>
#include <avr/sleep.h>




#define buttonPause A0
#define buttonUp A1
#define buttonDown A2
#define busyPin 4
#define shutdownPin 7
#define openAnalogPin A6

#define LONG_PRESS 1000

extern uint16_t currentTrack;
extern uint16_t numTracksInFolder;
extern uint16_t firstTrack;

static Button pauseButton(buttonPause);
static Button upButton(buttonUp);
static Button downButton(buttonDown);

// admin settings stored in eeprom
struct adminSettings {
  uint32_t cookie;
  byte version;
  uint8_t maxVolume;
  uint8_t minVolume;
  uint8_t initVolume;
  uint8_t eq;
  bool locked;
  long standbyTimer;
  bool invertVolumeButtons;
  folderSettings shortCuts[4];
  uint8_t adminMenuLocked;
  uint8_t adminMenuPin[4];
  bool stopWhenCardAway;
};

extern SoftwareSerial mySoftwareSerial; // RX, TX;
static DFMiniMp3<SoftwareSerial, Mp3Notify> mp3(mySoftwareSerial);
//
extern adminSettings mySettings;

extern nfcTagObject myCard;
extern folderSettings *myFolder;
extern unsigned long sleepAtMillis;

extern bool knownCard;

void setstandbyTimer();
bool setupFolder(folderSettings * theFolder);
void adminMenu(bool fromCard = false);
uint8_t voiceMenu(int numberOfOptions, int startMessage, int messageOffset, bool preview = false, int previewFromFolder = 0, int defaultValue = 0, bool exitWithLongPress = false);

void initPins();
void handleButtons();
void readPotentiometer();

#endif
