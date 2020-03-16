#ifndef player_h
#define player_h

#include "Arduino.h"
// include SPI, MP3 and SD libraries
#include <SPI.h>
#include <SD.h>
#include "Adafruit_VS1053.h"


  #define VS1053_CS      6     // VS1053 chip select pin (output)        - D6
  #define VS1053_DCS     7     // VS1053 Data/command select pin (output)- D7
  #define CARDCS         5     // Card chip select pin                   - D9
  #define VS1053_DREQ    2     // VS1053 Data request, ideally an Interrupt pin - D2
  #define VS1053_RESET   4     // VS1053 reset pin 

static uint16_t numTracksInFolder;
static uint16_t currentTrack;
static uint16_t firstTrack;
static uint8_t queue[255];
static uint8_t volume;
static uint16_t _lastTrackFinished=0;

static Adafruit_VS1053_FilePlayer musicPlayer = 
  Adafruit_VS1053_FilePlayer(VS1053_RESET, VS1053_CS, VS1053_DCS, VS1053_DREQ, CARDCS);

bool isPlaying();
void playShortCut(uint8_t shortCut);
void waitForTrackToFinish();
    //void previousTrack();
    //void nextTrack(uint16_t track);
void shuffleQueue();


//TODO: write progress not to EEPROM.update, but to card itself: folderSettings.lastPos every minute

class Player 
{
  public:
    Player();
    void init();
    void loop();
    void playFolderTrack(uint8_t folder, uint16_t track);//TODO: folder and track can be string
    void playFolder();      //executed only on new card and shortcut
    void playMp3FolderTrack(uint16_t track); //play track from the mp3 folder
    void pause();
    void start(); //resume where it was stopped
    void nextTrack();
    void previousTrack();
    void waitForTrackToFinish();
    //void shuffleQueue();
    void seekForward();
    void seekBackward();
    void playerHasPaused();
    void playerHasResumed();
    void playerHasStopped();
    void setVolume(uint8_t volume);
    //void mp3.setEq(mySettings.eq - 1);
    uint16_t getFolderTrackCount(char *folder);
    void playAdvertisement(uint16_t track);
    //void playShortcut
    

  private:
    void playFile(char *filename);
    uint16_t currentTrack;
    String *listOfTracks;
    //uint16_t numTracksInFolder;
    uint8_t currentFolder;
    String numToTrack(uint16_t num);
    uint32_t lastPos;
    uint16_t secondsPlayed;
    bool isAdvert;
    char fileName[64];
    
    
};




#endif
