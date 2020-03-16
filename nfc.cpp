#include "nfc.h"

MFRC522 mfrc522(10, 9); // Create MFRC522
uint32_t cardCookie = 322417479;
MFRC522::MIFARE_Key key;

bool successRead;
byte sector = 1;
byte blockAddr = 4;
byte trailerBlock = 5;


void initNFC()
{
  SPI.begin();        // Init SPI bus
  mfrc522.PCD_Init(); // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial(); // Show details of PCD - MFRC522 Card Reader
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

//Um festzustellen ob eine Karte entfernt wurde, muss der MFRC regelmäßig ausgelesen werden
byte pollCard()
{
  const byte maxRetries = 2;

  if (!hasCard)
  {
    if (mfrc522.PICC_IsNewCardPresent())
    {
      if (mfrc522.PICC_ReadCardSerial())
      {
        Serial.print(F("ReadCardSerial finished"));
        if (readCard(&myCard))
        {
      bool bSameUID = !memcmp(lastCardUid, mfrc522.uid.uidByte, 4);
      if (bSameUID) {
        Serial.print(F("Gleiche Karte"));
      }
      else {
        Serial.print(F("Neue Karte"));
      }
      // store info about current card
      memcpy(lastCardUid, mfrc522.uid.uidByte, 4);
      lastCardWasUL = mfrc522.PICC_GetType(mfrc522.uid.sak) == MFRC522::PICC_TYPE_MIFARE_UL;
    
      retries = maxRetries;
      hasCard = true;
      return bSameUID ? PCS_CARD_IS_BACK : PCS_NEW_CARD;
          }
          else //readCard war nicht erfolgreich
          {
            mfrc522.PICC_HaltA();
            mfrc522.PCD_StopCrypto1();
            Serial.print(F("Karte konnte nicht gelesen werden"));
          }
      }
    }
    return PCS_NO_CHANGE;
  }
  else // hasCard
  {
    // perform a dummy read command just to see whether the card is in range
    byte buffer[18];
    byte size = sizeof(buffer);
    
    if (mfrc522.MIFARE_Read(lastCardWasUL ? 8 : blockAddr, buffer, &size) != MFRC522::STATUS_OK)
    {
      if (retries > 0)
      {
          retries--;
      }
      else
      {
          Serial.println(F("Karte ist weg!"));
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
          hasCard = false;
          return PCS_CARD_GONE;
      }
    }
    else
    {
        retries = maxRetries;
    }
  }
  return PCS_NO_CHANGE;

}

void handleCardReader()
{
  // poll card only every 100ms
  static uint8_t lastCardPoll = 0;
  uint8_t now = millis();
  
  if (static_cast<uint8_t>(now - lastCardPoll) > 100)
  {
    lastCardPoll = now;
    switch (pollCard())
    {
    case PCS_NEW_CARD:
      onNewCard();
      break;
    case PCS_CARD_GONE:
    if (mySettings.stopWhenCardAway) 
    {
      mp3.pause();
      setstandbyTimer();
    }
      break;
      
    case PCS_CARD_IS_BACK:
    if (mySettings.stopWhenCardAway) 
    {
      //nur weiterspielen wenn vorher nicht konfiguriert wurde
      if (!forgetLastCard) 
      {
          mp3.start();
          disablestandbyTimer();
      }
      else 
      {
          onNewCard();
      }
    }

      break;
    }    
  }

}

void onNewCard()
{
   forgetLastCard=false;
  // make random a little bit more "random"
  randomSeed(millis() + random(1000));
    if (myCard.cookie == cardCookie && myCard.nfcFolderSettings.folder != 0 && myCard.nfcFolderSettings.mode != 0) {
      mp3.playFolder();
  }

    // Neue Karte konfigurieren
    else if (myCard.cookie != cardCookie) {
      knownCard = false;
      mp3.playMp3FolderTrack(300);
      waitForTrackToFinish();
      setupCard();
    }
}

void setupCard() {
  mp3.pause();
  Serial.println(F("=== setupCard()"));
  nfcTagObject newCard;
  if (setupFolder(&newCard.nfcFolderSettings) == true)
  {
    // Karte ist konfiguriert -> speichern
    mp3.pause();
    do {
    } while (isPlaying());
    writeCard(newCard);
    forgetLastCard=true;
  }
  delay(1000);
}

MFRC522::StatusCode authenticate(MFRC522::PICC_Type mifareType)
{



  // Authenticate using key B
  //authentificate with the card and set card specific parameters
  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI ) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K ) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K ) )
  {
    Serial.println(F("Authenticating again using key A..."));
    status = mfrc522.PCD_Authenticate(
               MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
  }
  else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL )
  {
    byte pACK[] = {0, 0}; //16 bit PassWord ACK returned by the NFCtag

    // Authenticate using key A
    Serial.println(F("Authenticating UL..."));
    status = mfrc522.PCD_NTAG216_AUTH(key.keyByte, pACK);
  }
  return status;
  
}


bool readCard(nfcTagObject * nfcTag) {
  nfcTagObject tempCard;
  // Show some details of the PICC (that is: the tag/card)
  Serial.print(F("Card UID:"));
  dump_byte_array(mfrc522.uid.uidByte, mfrc522.uid.size);
  Serial.println();
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  Serial.println(mfrc522.PICC_GetTypeName(piccType));

  byte buffer[18];
  byte size = sizeof(buffer);

  status=authenticate(piccType);

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    return false;
  }

  // Show the whole sector as it currently is
  // Serial.println(F("Current data in sector:"));
  // mfrc522.PICC_DumpMifareClassicSectorToSerial(&(mfrc522.uid), &key, sector);
  // Serial.println();

  // Read data from the block
  if ((piccType == MFRC522::PICC_TYPE_MIFARE_MINI ) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_1K ) ||
      (piccType == MFRC522::PICC_TYPE_MIFARE_4K ) )
  {
    Serial.print(F("Reading data from block "));
    Serial.print(blockAddr);
    Serial.println(F(" ..."));
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(blockAddr, buffer, &size);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
  }
  else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL )
  {
    byte buffer2[18];
    byte size2 = sizeof(buffer2);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(8, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_1() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(9, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_2() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 4, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(10, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_3() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 8, buffer2, 4);

    status = (MFRC522::StatusCode)mfrc522.MIFARE_Read(11, buffer2, &size2);
    if (status != MFRC522::STATUS_OK) {
      Serial.print(F("MIFARE_Read_4() failed: "));
      Serial.println(mfrc522.GetStatusCodeName(status));
      return false;
    }
    memcpy(buffer + 12, buffer2, 4);

  }

  Serial.print(F("Data on Card "));
  Serial.println(F(":"));
  dump_byte_array(buffer, 16);
  Serial.println();
  Serial.println();

  uint32_t tempCookie;
  tempCookie = (uint32_t)buffer[0] << 24;
  tempCookie += (uint32_t)buffer[1] << 16;
  tempCookie += (uint32_t)buffer[2] << 8;
  tempCookie += (uint32_t)buffer[3];

  tempCard.cookie = tempCookie;
  tempCard.version = buffer[4];
  tempCard.nfcFolderSettings.folder = buffer[5];
  tempCard.nfcFolderSettings.mode = buffer[6];
  tempCard.nfcFolderSettings.special = buffer[7];
  tempCard.nfcFolderSettings.special2 = buffer[8];
  tempCard.nfcFolderSettings.lastPos=(uint16_t)buffer[11] << 8 || (uint16_t)buffer[12]; 
  Serial.print("Las pos on Card: ");
  Serial.println(tempCard.nfcFolderSettings.lastPos);

  if (tempCard.cookie == cardCookie) {

    if (activeModifier != NULL && tempCard.nfcFolderSettings.folder != 0) {
      if (activeModifier->handleRFID(&tempCard) == true) {
        return false;
      }
    }

    if (tempCard.nfcFolderSettings.folder == 0) {
      if (activeModifier != NULL) {
        if (activeModifier->getActive() == tempCard.nfcFolderSettings.mode) {
          activeModifier = NULL;
          Serial.println(F("modifier removed"));
          if (isPlaying()) {
            mp3.playAdvertisement(261);
          }
          else {
            mp3.start();
            delay(100);
            mp3.playAdvertisement(261);
            delay(100);
            mp3.pause();
          }
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
          delay(2000);
          return false;
        }
      }
      if (tempCard.nfcFolderSettings.mode != 0 && tempCard.nfcFolderSettings.mode != 255) {
        Serial.println(F("modifier active"));
        if (isPlaying()) {
          mp3.playAdvertisement(260);
        }
        else {
          mp3.start();
          delay(100);
          mp3.playAdvertisement(260);
          delay(100);
          mp3.pause();
        }
      }
      switch (tempCard.nfcFolderSettings.mode ) {
        case 0:
        case 255:
          mfrc522.PICC_HaltA(); mfrc522.PCD_StopCrypto1(); adminMenu(true);  break;
        case 1: activeModifier = new SleepTimer(tempCard.nfcFolderSettings.special); break;
//        case 2: activeModifier = new FreezeDance(); break;
        case 3: activeModifier = new Locked(); break;
        //case 4: activeModifier = new ToddlerMode(); break;
//        case 5: activeModifier = new KindergardenMode(); break;
//        case 6: activeModifier = new RepeatSingleModifier(); break;
      }
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      delay(2000);
      return false;
    }
    else {
      memcpy(nfcTag, &tempCard, sizeof(nfcTagObject));
      Serial.println( nfcTag->nfcFolderSettings.folder);
      myFolder = &nfcTag->nfcFolderSettings;
      Serial.println( myFolder->folder);
    }
    return true;
  }
  else {
    memcpy(nfcTag, &tempCard, sizeof(nfcTagObject));
    return true;
  }
}


void writeCard(nfcTagObject nfcTag) {
  MFRC522::PICC_Type mifareType;
    mifareType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  byte buffer[16] = {0x13, 0x37, 0xb3, 0x47, // 0x1337 0xb347 magic cookie to
                     // identify our nfc tags
                     0x02,                   // version 1
                     nfcTag.nfcFolderSettings.folder,          // the folder picked by the user
                     nfcTag.nfcFolderSettings.mode,    // the playback mode picked by the user
                     nfcTag.nfcFolderSettings.special, // track or function for admin cards
                     nfcTag.nfcFolderSettings.special2,
                     nfcTag.nfcFolderSettings.lastPos,// 
                     0x00, 0x00, 0x00, 0x00, 0x00
                    };

  byte size = sizeof(buffer);
  status = authenticate(mifareType);

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("PCD_Authenticate() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mp3.playMp3FolderTrack(401);
    return;
  }

  // Write data to the block
  Serial.print(F("Writing data into block "));
  Serial.print(blockAddr);
  Serial.println(F(" ..."));
  dump_byte_array(buffer, size);
  Serial.println();

  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI ) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K ) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K ) )
  {
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr, buffer, size);
  }
  else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL )
  {
    byte buffer2[size];
    byte size2 = sizeof(buffer2);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(8, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 4, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(9, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 8, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(10, buffer2, 16);

    memset(buffer2, 0, size2);
    memcpy(buffer2, buffer + 12, 4);
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(11, buffer2, 16);

  }

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));
    mp3.playMp3FolderTrack(401);
  }
  else
    mp3.playMp3FolderTrack(400);
  Serial.println();
  delay(2000);
}

void updatePosition(uint16_t lastPos) {
  MFRC522::PICC_Type mifareType;
  mifareType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  status = authenticate(mifareType);
  byte buffer[4];
  buffer[0]=(byte)(myFolder->special2);
  buffer[1]=(byte)(lastPos);
  buffer[2]=(byte)(lastPos<<8);
  buffer[3]=(byte)(0x0);
  
  if (status != MFRC522::STATUS_OK) 
  {
    Serial.print(F("PCD_Authenticate() failed, playback position not updated!"));
    Serial.println(mfrc522.GetStatusCodeName(status));

    return;
  }

  if ((mifareType == MFRC522::PICC_TYPE_MIFARE_MINI ) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_1K ) ||
      (mifareType == MFRC522::PICC_TYPE_MIFARE_4K ) )
  {
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(blockAddr+2, buffer, 4);
  }
  else if (mifareType == MFRC522::PICC_TYPE_MIFARE_UL )
  {
    status = (MFRC522::StatusCode)mfrc522.MIFARE_Write(10, buffer, 4);
  }

  if (status != MFRC522::STATUS_OK) {
    Serial.print(F("MIFARE_Write() failed: "));
    Serial.println(mfrc522.GetStatusCodeName(status));

  }
  else
    Serial.print(F("Last Position updated to: "));
    Serial.println(lastPos);
}

void resetCard() 
{
  mp3.playMp3FolderTrack(800);
  do {
    pauseButton.read();
    upButton.read();
    downButton.read();

    if (upButton.wasReleased() || downButton.wasReleased()) {
      Serial.print(F("Abgebrochen!"));
      mp3.playMp3FolderTrack(802);
      return;
    }
  } while (!mfrc522.PICC_IsNewCardPresent());

  if (!mfrc522.PICC_ReadCardSerial())
    return;

  Serial.print(F("Karte wird neu konfiguriert!"));
  setupCard();
}
