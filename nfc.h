#ifndef nfc_h
#define nfc_h

#include "Arduino.h"
#include "TonUINO.h"
#include <MFRC522.h>

// MFRC522
//#define RST_PIN 9                 // Configurable, see typical pin layout above
//#define SS_PIN 10                 // Configurable, see typical pin layout above

extern MFRC522 mfrc522;//(SS_PIN, RST_PIN); // Create MFRC522
extern uint32_t cardCookie;

static MFRC522::StatusCode status;

void initNFC();

//StopWhenCardAway settings
static bool hasCard = false;
static byte lastCardUid[4];
static byte retries;
static bool lastCardWasUL;
static bool forgetLastCard=false;

const byte PCS_NO_CHANGE     = 0; // no change detected since last pollCard() call
const byte PCS_NEW_CARD      = 1; // card with new UID detected (had no card or other card before)
const byte PCS_CARD_GONE     = 2; // card is not reachable anymore
const byte PCS_CARD_IS_BACK  = 3; // card was gone, and is now back again

MFRC522::StatusCode authenticate(MFRC522::PICC_Type mifareType);
void setupCard();
bool readCard(nfcTagObject * nfcTag);
void writeCard(nfcTagObject nfcTag);
void resetCard();
void handleCardReader();
byte pollCard();
void onNewCard();

#endif 
