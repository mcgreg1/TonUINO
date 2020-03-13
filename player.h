#ifndef player_h
#define player_h

#include "Arduino.h"
#include <DFMiniMp3.h>

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
    static void PrintlnSourceAction(DfMp3_PlaySources source, const char* action);
    static void OnPlayFinished(DfMp3_PlaySources source, uint16_t track);
    static void OnPlaySourceOnline(DfMp3_PlaySources source); 
    static void OnPlaySourceInserted(DfMp3_PlaySources source); 
    static void OnPlaySourceRemoved(DfMp3_PlaySources source); 
};

void playFolder();
void playShortCut(uint8_t shortCut);
void previousTrack();
void nextTrack(uint16_t track);
bool isPlaying();
void waitForTrackToFinish();
void shuffleQueue();
#endif
