#ifndef utilities_h
#define utilities_h

#include "TonUINO.h"
void setstandbyTimer();
void disablestandbyTimer();
void checkStandbyAtMillis();

void dump_byte_array(byte * buffer, byte bufferSize);
bool checkTwo ( uint8_t a[], uint8_t b[] );

void loadSettingsFromFlash();
void migrateSettings(int oldVersion);
void resetSettings() ;
void writeSettingsToFlash();
void printLogo();
void initRandomSeed();
void printDirectory(File dir, int numTabs);
void findFile(File dir, uint16_t num, char *fn, byte number_length=3);
uint16_t countFiles(File dir);
//void playFolder();
#endif
