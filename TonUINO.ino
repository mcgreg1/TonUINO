#include "TonUINO.h"

/*
   _____         _____ _____ _____ _____
  |_   _|___ ___|  |  |     |   | |     |
    | | | . |   |  |  |-   -| | | |  |  |
    |_| |___|_|_|_____|_____|_|___|_____|
    TonUINO Version 2.1 + McGreg changes v0.1

    created by Thorsten Voß and licensed under GNU/GPL.
    Information and contribution at https://tonuino.de.
*/

// uncomment the below line to enable volume control with a potentiometer
//#define POTI


// DFPlayer Mini
SoftwareSerial mySoftwareSerial(2, 3); // RX, TX

adminSettings mySettings;
nfcTagObject myCard;
folderSettings *myFolder;
unsigned long sleepAtMillis = 0;
//static uint16_t _lastTrackFinished;
#define POTI

#ifdef POTI
#define POTIPIN 7
uint16_t potiValueRaw;
uint16_t oldPotiValueRaw;
byte potiValue;
byte oldPotiValue;
#endif

bool knownCard = false;

Modifier *activeModifier = NULL;

bool ignorePauseButton = false;
bool ignoreUpButton = false;
bool ignoreDownButton = false;

void setup() {

  Serial.begin(115200); // Es gibt ein paar Debug Ausgaben über die serielle Schnittstelle
   
  initRandomSeed();
  // Dieser Hinweis darf nicht entfernt werden
  printLogo();
  initPins();
  initPlayer();

  // load Settings from EEPROM
  loadSettingsFromFlash();

  // activate standby timer
  setstandbyTimer();
  
  // NFC Leser initialisieren
  initNFC();

  // RESET --- ALLE DREI KNÖPFE BEIM STARTEN GEDRÜCKT HALTEN -> alle EINSTELLUNGEN werden gelöscht
  if (digitalRead(buttonPause) == LOW && digitalRead(buttonUp) == LOW &&
      digitalRead(buttonDown) == LOW) {
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.update(i, 0);
    }
    loadSettingsFromFlash();
  }

  // Start Shortcut "at Startup" - e.g. Welcome Sound
  playShortCut(3);
}


void loop() {

    checkStandbyAtMillis();
    mp3.loop();

    // Modifier : WIP!
    if (activeModifier != NULL) 
    {
      if (!isPlaying()) 
      {
        mp3.start();
      }
      activeModifier->loop();
    }
    // Buttons werden nun über JS_Button gehandelt, dadurch kann jede Taste
    // doppelt belegt werden
    readButtons();
    handleButtons();

    #ifdef POTI
    readPotentiometer();
    #endif
    
    handleCardReader();
}

void initPins()
{
    // Busy Pin
  pinMode(busyPin, INPUT);
    #ifdef POTI
  potiValueRaw = analogRead(POTIPIN);
  volume = map(potiValueRaw, 0,1024,mySettings.minVolume, mySettings.maxVolume+1);
  Serial.print(F("Poti Bereich: ")); Serial.print(mySettings.minVolume); Serial.print(" - "); Serial.println(mySettings.maxVolume+1);
  Serial.println(F("Initiale Lautstärke über Poti:"));
  Serial.println(volume);
  #endif
  
  pinMode(buttonPause, INPUT_PULLUP);
  pinMode(buttonUp, INPUT_PULLUP);
  pinMode(buttonDown, INPUT_PULLUP);

  pinMode(shutdownPin, OUTPUT);
  digitalWrite(shutdownPin, LOW);
}

void readButtons() 
{
  pauseButton.read();
  upButton.read();
  downButton.read();
}

void volumeUpButton() 
{
  #ifndef POTI
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeUp() == true)
      return;

  Serial.println(F("=== volumeUp()"));
  if (volume < mySettings.maxVolume) 
  {
    mp3.increaseVolume();
    volume++;
  }
  Serial.println(volume);
  #endif
  #ifdef POTI
  nextButton();
  #endif
}

void volumeDownButton() 
{
  #ifndef POTI
  if (activeModifier != NULL)
    if (activeModifier->handleVolumeDown() == true)
      return;

  Serial.println(F("=== volumeDown()"));
  if (volume > mySettings.minVolume) {
    mp3.decreaseVolume();
    volume--;
  }
  Serial.println(volume);
  #endif
  #ifdef POTI
  previousButton();
  #endif
}

void nextButton() 
{
  if (activeModifier != NULL)
    if (activeModifier->handleNextButton() == true)
      return;

  nextTrack(random(65536));
  delay(1000);
}

void previousButton() 
{
  if (activeModifier != NULL)
    if (activeModifier->handlePreviousButton() == true)
      return;

  previousTrack();
  delay(1000);
}

#ifdef POTI
void readPotentiometer() 
{

  potiValueRaw = analogRead(POTIPIN);
  //Serial.println("Analog output Poti:");
  //Serial.println(PotiValue);
  if (potiValueRaw>>5 != oldPotiValueRaw>>5)
  {
  
    potiValue = map(potiValue, 0,1024,mySettings.minVolume, mySettings.maxVolume+1);
    if (potiValue!=oldPotiValue)
    //if (PotiValue > oldPotiValue || PotiValue < oldPotiValue ) 
    {
        oldPotiValue=potiValue;
        oldPotiValueRaw=potiValueRaw;
        if (activeModifier != NULL)
          if (activeModifier->handlePotiVolume() == true)
            return;  
      Serial.print(F("PotiLautstärke: "));
      Serial.println(potiValue);
      mp3.setVolume(potiValue);
      
    }
  }
}
#endif



void handleButtons()
{
      // admin menu
    if ((pauseButton.pressedFor(LONG_PRESS) || upButton.pressedFor(LONG_PRESS) || downButton.pressedFor(LONG_PRESS)) && pauseButton.isPressed() && upButton.isPressed() && downButton.isPressed()) {
      mp3.pause();
      do {
        readButtons();
      } while (pauseButton.isPressed() || upButton.isPressed() || downButton.isPressed());
      readButtons();
      adminMenu();
      return;
    }

    if (pauseButton.wasReleased()) 
    {
      if (activeModifier != NULL)
        if (activeModifier->handlePause() == true)
          return;
      if (ignorePauseButton == false)
        if (isPlaying()) {
          mp3.pause();
          setstandbyTimer();
        }
        else if (knownCard) {
          mp3.start();
          disablestandbyTimer();
        }
      ignorePauseButton = false;
    } 
    else if (pauseButton.pressedFor(LONG_PRESS) && ignorePauseButton == false) 
    {
      if (activeModifier != NULL)
        if (activeModifier->handlePause() == true)
          return;
      if (isPlaying()) {
        uint8_t advertTrack;
        if (myFolder->mode == 3 || myFolder->mode == 9) {
          advertTrack = (queue[currentTrack - 1]);
        }
        else {
          advertTrack = currentTrack;
        }
        // Spezialmodus Von-Bis für Album und Party gibt die Dateinummer relativ zur Startposition wieder
        if (myFolder->mode == 8 || myFolder->mode == 9) {
          advertTrack = advertTrack - myFolder->special + 1;
        }
        mp3.playAdvertisement(advertTrack);
      }
      else {
        playShortCut(0);
      }
      ignorePauseButton = true;
    }

    if (upButton.pressedFor(LONG_PRESS)) {
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeUpButton();
        }
        else {
          nextButton();
        }
      }
      else {
        playShortCut(1);
      }
      ignoreUpButton = true;
    } else if (upButton.wasReleased()) {
      if (!ignoreUpButton)
        if (!mySettings.invertVolumeButtons) {
          nextButton();
        }
        else {
          volumeUpButton();
        }
      ignoreUpButton = false;
    }

    if (downButton.pressedFor(LONG_PRESS)) {
      if (isPlaying()) {
        if (!mySettings.invertVolumeButtons) {
          volumeDownButton();
        }
        else {
          previousButton();
        }
      }
      else {
        playShortCut(2);
      }
      ignoreDownButton = true;
    } else if (downButton.wasReleased()) 
    {
      if (!ignoreDownButton) {
        if (!mySettings.invertVolumeButtons) {
          previousButton();
        }
        else {
          volumeDownButton();
        }
      }
      ignoreDownButton = false;
    }
    // Ende der Buttons
}

void adminMenu(bool fromCard = false) 
{
  //Vergesse die vorherige Karte, wenn das Admin Menü betreten wird
  forgetLastCard=true;
  disablestandbyTimer();
  mp3.pause();
  Serial.println(F("=== adminMenu()"));
  knownCard = false;
  if (fromCard == false) {
    // Admin menu has been locked - it still can be trigged via admin card
    if (mySettings.adminMenuLocked == 1) {
      return;
    }
    // Pin check
    else if (mySettings.adminMenuLocked == 2) {
      uint8_t pin[4];
      mp3.playMp3FolderTrack(991);
      if (askCode(pin) == true) {
        if (checkTwo(pin, mySettings.adminMenuPin) == false) {
          return;
        }
      } else {
        return;
      }
    }
    // Match check
    else if (mySettings.adminMenuLocked == 3) {
      uint8_t a = random(10, 20);
      uint8_t b = random(1, 10);
      uint8_t c;
      mp3.playMp3FolderTrack(992);
      waitForTrackToFinish();
      mp3.playMp3FolderTrack(a);

      if (random(1, 3) == 2) {
        // a + b
        c = a + b;
        waitForTrackToFinish();
        mp3.playMp3FolderTrack(993);
      } else {
        // a - b
        b = random(1, a);
        c = a - b;
        waitForTrackToFinish();
        mp3.playMp3FolderTrack(994);
      }
      waitForTrackToFinish();
      mp3.playMp3FolderTrack(b);
      Serial.println(c);
      uint8_t temp = voiceMenu(255, 0, 0, false);
      if (temp != c) {
        return;
      }
    }
  }
  int subMenu = voiceMenu(13, 900, 900, false, false, 0, true);
  if (subMenu == 0)
    return;
  if (subMenu == 1) {
    resetCard();
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }
  else if (subMenu == 2) {
    // Maximum Volume
    mySettings.maxVolume = voiceMenu(30 - mySettings.minVolume, 930, mySettings.minVolume, false, false, mySettings.maxVolume - mySettings.minVolume) + mySettings.minVolume;
  }
  else if (subMenu == 3) {
    // Minimum Volume
    mySettings.minVolume = voiceMenu(mySettings.maxVolume - 1, 931, 0, false, false, mySettings.minVolume);
  }
  else if (subMenu == 4) {
    // Initial Volume
    mySettings.initVolume = voiceMenu(mySettings.maxVolume - mySettings.minVolume + 1, 932, mySettings.minVolume - 1, false, false, mySettings.initVolume - mySettings.minVolume + 1) + mySettings.minVolume - 1;
  }
  else if (subMenu == 5) {
    // EQ
    mySettings.eq = voiceMenu(6, 920, 920, false, false, mySettings.eq);
    mp3.setEq(mySettings.eq - 1);
  }
  else if (subMenu == 6) {
    // create modifier card
    nfcTagObject tempCard;
    tempCard.cookie = cardCookie;
    tempCard.version = 1;
    tempCard.nfcFolderSettings.folder = 0;
    tempCard.nfcFolderSettings.special = 0;
    tempCard.nfcFolderSettings.special2 = 0;
    tempCard.nfcFolderSettings.mode = voiceMenu(6, 970, 970, false, false, 0, true);

    if (tempCard.nfcFolderSettings.mode != 0) {
      if (tempCard.nfcFolderSettings.mode == 1) {
        switch (voiceMenu(4, 960, 960)) {
          case 1: tempCard.nfcFolderSettings.special = 5; break;
          case 2: tempCard.nfcFolderSettings.special = 15; break;
          case 3: tempCard.nfcFolderSettings.special = 30; break;
          case 4: tempCard.nfcFolderSettings.special = 60; break;
        }
      }
      mp3.playMp3FolderTrack(800);
      do {
        readButtons();
        if (upButton.wasReleased() || downButton.wasReleased()) {
          Serial.println(F("Abgebrochen!"));
          mp3.playMp3FolderTrack(802);
          return;
        }
      } while (!mfrc522.PICC_IsNewCardPresent());

      // RFID Karte wurde aufgelegt
      if (mfrc522.PICC_ReadCardSerial()) {
        Serial.println(F("schreibe Karte..."));
        writeCard(tempCard);
        delay(100);
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        waitForTrackToFinish();
      }
    }
  }
  else if (subMenu == 7) {
    uint8_t shortcut = voiceMenu(4, 940, 940);
    setupFolder(&mySettings.shortCuts[shortcut - 1]);
    mp3.playMp3FolderTrack(400);
  }
  else if (subMenu == 8) {
    switch (voiceMenu(5, 960, 960)) {
      case 1: mySettings.standbyTimer = 5; break;
      case 2: mySettings.standbyTimer = 15; break;
      case 3: mySettings.standbyTimer = 30; break;
      case 4: mySettings.standbyTimer = 60; break;
      case 5: mySettings.standbyTimer = 0; break;
    }
  }
  else if (subMenu == 9) {
    // Create Cards for Folder
    // Ordner abfragen
    nfcTagObject tempCard;
    tempCard.cookie = cardCookie;
    tempCard.version = 1;
    tempCard.nfcFolderSettings.mode = 4;
    tempCard.nfcFolderSettings.folder = voiceMenu(99, 301, 0, true);
    uint8_t special = voiceMenu(mp3.getFolderTrackCount(tempCard.nfcFolderSettings.folder), 321, 0,
                                true, tempCard.nfcFolderSettings.folder);
    uint8_t special2 = voiceMenu(mp3.getFolderTrackCount(tempCard.nfcFolderSettings.folder), 322, 0,
                                 true, tempCard.nfcFolderSettings.folder, special);

    mp3.playMp3FolderTrack(936);
    waitForTrackToFinish();
    for (uint8_t x = special; x <= special2; x++) {
      mp3.playMp3FolderTrack(x);
      tempCard.nfcFolderSettings.special = x;
      Serial.print(x);
      Serial.println(F(" Karte auflegen"));
      do {
        readButtons();
        if (upButton.wasReleased() || downButton.wasReleased()) {
          Serial.println(F("Abgebrochen!"));
          mp3.playMp3FolderTrack(802);
          return;
        }
      } while (!mfrc522.PICC_IsNewCardPresent());

      // RFID Karte wurde aufgelegt
      if (mfrc522.PICC_ReadCardSerial()) {
        Serial.println(F("schreibe Karte..."));
        writeCard(tempCard);
        delay(100);
        mfrc522.PICC_HaltA();
        mfrc522.PCD_StopCrypto1();
        waitForTrackToFinish();
      }
    }
  }
  else if (subMenu == 10) {
    // Invert Functions for Up/Down Buttons
    int temp = voiceMenu(2, 933, 933, false);
    if (temp == 2) {
      mySettings.invertVolumeButtons = true;
    }
    else {
      mySettings.invertVolumeButtons = false;
    }
  }
  else if (subMenu == 11) {
    Serial.println(F("Reset -> EEPROM wird gelöscht"));
    for (int i = 0; i < EEPROM.length(); i++) {
      EEPROM.update(i, 0);
    }
    resetSettings();
    mp3.playMp3FolderTrack(999);
  }
  // lock admin menu
  else if (subMenu == 12) {
    int temp = voiceMenu(4, 980, 980, false);
    if (temp == 1) {
      mySettings.adminMenuLocked = 0;
    }
    else if (temp == 2) {
      mySettings.adminMenuLocked = 1;
    }
    else if (temp == 3) {
      int8_t pin[4];
      mp3.playMp3FolderTrack(991);
      if (askCode(pin)) {
        memcpy(mySettings.adminMenuPin, pin, 4);
        mySettings.adminMenuLocked = 2;
      }
    }
    else if (temp == 4) {
      mySettings.adminMenuLocked = 3;
    }

  }
  //Wiedergabe stoppen wenn Karte entfernt wird
  else if (subMenu == 13) {
    int temp = voiceMenu(2, 937, 937, false);
    if (temp == 2) {
      mySettings.stopWhenCardAway = true;
      Serial.println(F("StopWhenCardAway --> yes"));
    }
    else {
      mySettings.stopWhenCardAway = false;
      Serial.println(F("StopWhenCardAway --> no"));
    }
  }
  writeSettingsToFlash();
  setstandbyTimer();
}

bool askCode(uint8_t *code) 
{
  uint8_t x = 0;
  while (x < 4) {
    readButtons();
    if (pauseButton.pressedFor(LONG_PRESS))
      break;
    if (pauseButton.wasReleased())
      code[x++] = 1;
    if (upButton.wasReleased())
      code[x++] = 2;
    if (downButton.wasReleased())
      code[x++] = 3;
  }
  return true;
}

uint8_t voiceMenu(int numberOfOptions, int startMessage, int messageOffset,
                  bool preview = false, int previewFromFolder = 0, int defaultValue = 0, bool exitWithLongPress = false) 
{
  uint8_t returnValue = defaultValue;
  if (startMessage != 0)
    mp3.playMp3FolderTrack(startMessage);
  Serial.print(F("=== voiceMenu() ("));
  Serial.print(numberOfOptions);
  Serial.println(F(" Options)"));
  do {
    if (Serial.available() > 0) {
      int optionSerial = Serial.parseInt();
      if (optionSerial != 0 && optionSerial <= numberOfOptions)
        return optionSerial;
    }
    readButtons();
    mp3.loop();
    if (pauseButton.pressedFor(LONG_PRESS)) {
      mp3.playMp3FolderTrack(802);
      ignorePauseButton = true;
      return defaultValue;
    }
    if (pauseButton.wasReleased()) {
      if (returnValue != 0) {
        Serial.print(F("=== "));
        Serial.print(returnValue);
        Serial.println(F(" ==="));
        return returnValue;
      }
      delay(1000);
    }

    if (upButton.pressedFor(LONG_PRESS)) {
      returnValue = min(returnValue + 10, numberOfOptions);
      Serial.println(returnValue);
      //mp3.pause();
      mp3.playMp3FolderTrack(messageOffset + returnValue);
      waitForTrackToFinish();
      /*if (preview) {
        if (previewFromFolder == 0)
          mp3.playFolderTrack(returnValue, 1);
        else
          mp3.playFolderTrack(previewFromFolder, returnValue);
        }*/
      ignoreUpButton = true;
    } else if (upButton.wasReleased()) {
      if (!ignoreUpButton) {
        returnValue = min(returnValue + 1, numberOfOptions);
        Serial.println(returnValue);
        //mp3.pause();
        mp3.playMp3FolderTrack(messageOffset + returnValue);
        if (preview) {
          waitForTrackToFinish();
          if (previewFromFolder == 0) {
            mp3.playFolderTrack(returnValue, 1);
          } else {
            mp3.playFolderTrack(previewFromFolder, returnValue);
          }
          delay(1000);
        }
      } else {
        ignoreUpButton = false;
      }
    }

    if (downButton.pressedFor(LONG_PRESS)) {
      returnValue = max(returnValue - 10, 1);
      Serial.println(returnValue);
      //mp3.pause();
      mp3.playMp3FolderTrack(messageOffset + returnValue);
      waitForTrackToFinish();
      /*if (preview) {
        if (previewFromFolder == 0)
          mp3.playFolderTrack(returnValue, 1);
        else
          mp3.playFolderTrack(previewFromFolder, returnValue);
        }*/
      ignoreDownButton = true;
    } else if (downButton.wasReleased()) {
      if (!ignoreDownButton) {
        returnValue = max(returnValue - 1, 1);
        Serial.println(returnValue);
        //mp3.pause();
        mp3.playMp3FolderTrack(messageOffset + returnValue);
        if (preview) {
          waitForTrackToFinish();
          if (previewFromFolder == 0) {
            mp3.playFolderTrack(returnValue, 1);
          }
          else {
            mp3.playFolderTrack(previewFromFolder, returnValue);
          }
          delay(1000);
        }
      } else {
        ignoreDownButton = false;
      }
    }
  } while (true);
}



bool setupFolder(folderSettings * theFolder) 
{
  // Ordner abfragen
  theFolder->folder = voiceMenu(99, 301, 0, true, 0, 0, true);
  if (theFolder->folder == 0) return false;

  // Wiedergabemodus abfragen
  theFolder->mode = voiceMenu(9, 310, 310, false, 0, 0, true);
  if (theFolder->mode == 0) return false;

  //  // Hörbuchmodus -> Fortschritt im EEPROM auf 1 setzen
  //  EEPROM.update(theFolder->folder, 1);

  // Einzelmodus -> Datei abfragen
  if (theFolder->mode == 4)
    theFolder->special = voiceMenu(mp3.getFolderTrackCount(theFolder->folder), 320, 0,
                                   true, theFolder->folder);
  // Admin Funktionen
  if (theFolder->mode == 6) {
    //theFolder->special = voiceMenu(3, 320, 320);
    theFolder->folder = 0;
    theFolder->mode = 255;
  }
  // Spezialmodus Von-Bis
  if (theFolder->mode == 7 || theFolder->mode == 8 || theFolder->mode == 9) {
    theFolder->special = voiceMenu(mp3.getFolderTrackCount(theFolder->folder), 321, 0,
                                   true, theFolder->folder);
    theFolder->special2 = voiceMenu(mp3.getFolderTrackCount(theFolder->folder), 322, 0,
                                    true, theFolder->folder, theFolder->special);
  }
  return true;
}
