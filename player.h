#ifndef player_h
#define player_h

#include "Arduino.h"

static uint16_t numTracksInFolder;
static uint16_t currentTrack;
static uint16_t firstTrack;
static uint8_t queue[255];
static uint8_t volume;
static uint16_t _lastTrackFinished=0;

void initPlayer();

class Mp3Notify {
  public:
    static void OnError(uint16_t errorCode);
    static void OnPlayFinished(uint16_t track);
    static void OnCardOnline(uint16_t code);
    static void OnCardInserted(uint16_t code);
    static void OnCardRemoved(uint16_t code);
    static void OnUsbOnline(uint16_t code);
    static void OnUsbInserted(uint16_t code); 
    static void OnUsbRemoved(uint16_t code);
};

void playFolder();
void playShortCut(uint8_t shortCut);
void previousTrack();
void nextTrack(uint16_t track);
bool isPlaying();
void waitForTrackToFinish();
void shuffleQueue();
#endif
