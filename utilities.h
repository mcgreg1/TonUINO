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

#endif
