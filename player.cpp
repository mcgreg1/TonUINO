#include "player.h"
#include "TonUINO.h"

// implement a notification class,
// its member methods will get called
//


Player::Player()
{
          // Wait for serial port to be opened, remove this line for 'standalone' operation
        while (!Serial) { delay(1); }
        delay(500);
        Serial.println("\n\nAdafruit VS1053 Feather Test");

  
}
void Player::init()
{
  
        if (! musicPlayer.begin()) { // initialise the music player
           Serial.println(F("Couldn't find VS1053, do you have the right pins defined?"));
           while (1);
        }

        Serial.println(F("VS1053 found"));
       
        //musicPlayer.sineTest(0x44, 500);    // Make a tone to indicate VS1053 is working
        
        if (!SD.begin(CARDCS)) {
          Serial.println(F("SD failed, or not present"));
          while (1);  // don't do anything more
        }
        Serial.println("SD OK!");

          // list files
        printDirectory(SD.open("/"), 0);
        
        #ifndef POTI
        volume = mySettings.initVolume;
        #endif
        musicPlayer.setVolume(10,10);
}
void Player::loop()
{
  if (musicPlayer.stopped()) 
    playerHasStopped();
  if (musicPlayer.paused()) 
    playerHasPaused();
  else
    playerHasResumed();
}


void Player::playFile(char *filename)
{
  musicPlayer.startPlayingFile(filename);
}

void Player::pause()
{
  musicPlayer.pausePlaying(true);
}
void Player::playFolderTrack(uint8_t folder, uint16_t track)
{
  
  //String filename=numToTrack(track);
  findFile(SD.open((char*)(folder)), track, fileName);
  if (fileName!=NULL)
  {
    currentTrack=track;
    currentFolder=folder;
    musicPlayer.startPlayingFile(fileName);
  }
}
void Player::playMp3FolderTrack(uint16_t track)
{
  char fn[64];
  findFile(SD.open("mp3"), track, fn, 4);
  if (fn!=NULL)
  {
    musicPlayer.startPlayingFile(fn);
  }
}
void Player::playAdvertisement(uint16_t track)
{
  lastPos=musicPlayer.getFilePosition();
  musicPlayer.stopPlaying();
  isAdvert=true;
  char fn[64];
  findFile(SD.open("advert"), track, fn, 4);
  if (fn!=NULL)
  {
    musicPlayer.startPlayingFile(fn);
  }
}
void Player::start()//resume when was stopped
{
  
}
uint16_t Player::getFolderTrackCount(char *folder)
{
  return countFiles(SD.open(folder));
}
void Player::setVolume(uint8_t volume)
{
  musicPlayer.setVolume(volume,volume);
}

void Player::playerHasPaused()
{
  Serial.println("Player paused!");
}
void Player::playerHasResumed()
{
  Serial.println("Player resumed!");
}
void Player::playerHasStopped()
{
  //depending on the mode current actions needed
  if (isAdvert)
  {
    musicPlayer.startPlayingFile(fileName, lastPos);
    isAdvert=false;
  }
  
}
void Player::nextTrack() 
{
  if (activeModifier != NULL)
    if (activeModifier->handleNext() == true)
      return;

  if (currentTrack == _lastTrackFinished) {
    return;
  }
  _lastTrackFinished = currentTrack;

  if (knownCard == false)
    // Wenn eine neue Karte angelernt wird soll das Ende eines Tracks nicht
    // verarbeitet werden
    return;

  Serial.println(F("=== nextTrack()"));

  if (myFolder->mode == 1 || myFolder->mode == 7) {
    Serial.println(F("Hörspielmodus ist aktiv -> keinen neuen Track spielen"));
    setstandbyTimer();
    //    mp3.sleep(); // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
  
   
  }
  if (myFolder->mode == 2 || myFolder->mode == 8) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      mp3.playFolderTrack(myFolder->folder, currentTrack);
      Serial.print(F("Albummodus ist aktiv -> nächster Track: "));
      Serial.print(currentTrack);
    } else
      //      mp3.sleep();   // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
      setstandbyTimer();
    { }
  }
  if (myFolder->mode == 3 || myFolder->mode == 9) {
    if (currentTrack != numTracksInFolder - firstTrack + 1) {
      Serial.print(F("Party -> weiter in der Queue "));
      currentTrack++;
    } else {
      Serial.println(F("Ende der Queue -> beginne von vorne"));
      currentTrack = 1;
      //// Wenn am Ende der Queue neu gemischt werden soll bitte die Zeilen wieder aktivieren
      //     Serial.println(F("Ende der Queue -> mische neu"));
      //     shuffleQueue();
    }
    Serial.println(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }

  if (myFolder->mode == 4) {
    Serial.println(F("Einzel Modus aktiv -> Strom sparen"));
    //    mp3.sleep();      // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
    setstandbyTimer();
  }
  if (myFolder->mode == 5) {
    if (currentTrack != numTracksInFolder) {
      currentTrack = currentTrack + 1;
      Serial.print(F("Hörbuch Modus ist aktiv -> nächster Track und "
                     "Fortschritt speichern"));
      Serial.println(currentTrack);
      mp3.playFolderTrack(myFolder->folder, currentTrack);
      // Fortschritt im EEPROM abspeichern
      EEPROM.update(myFolder->folder, currentTrack);
    } else {
      //      mp3.sleep();  // Je nach Modul kommt es nicht mehr zurück aus dem Sleep!
      // Fortschritt zurück setzen
      EEPROM.update(myFolder->folder, 1);
      setstandbyTimer();
    }
  }
  delay(500);
  
}
void Player::previousTrack() 
{
  Serial.println(F("=== previousTrack()"));
  if (myFolder->mode == 2 || myFolder->mode == 8) {
    Serial.println(F("Albummodus ist aktiv -> vorheriger Track"));
    if (currentTrack != firstTrack) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  if (myFolder->mode == 3 || myFolder->mode == 9) {
    if (currentTrack != 1) {
      Serial.print(F("Party Modus ist aktiv -> zurück in der Qeueue "));
      currentTrack--;
    }
    else
    {
      Serial.print(F("Anfang der Queue -> springe ans Ende "));
      currentTrack = numTracksInFolder;
    }
    Serial.println(queue[currentTrack - 1]);
    mp3.playFolderTrack(myFolder->folder, queue[currentTrack - 1]);
  }
  if (myFolder->mode == 4) {
    Serial.println(F("Einzel Modus aktiv -> Track von vorne spielen"));
    mp3.playFolderTrack(myFolder->folder, currentTrack);
  }
  if (myFolder->mode == 5) {
    Serial.println(F("Hörbuch Modus ist aktiv -> vorheriger Track und "
                     "Fortschritt speichern"));
    if (currentTrack != 1) {
      currentTrack = currentTrack - 1;
    }
    mp3.playFolderTrack(myFolder->folder, currentTrack);
    // Fortschritt im EEPROM abspeichern
    EEPROM.update(myFolder->folder, currentTrack);
  }
  delay(1000);
}
    void Player::playFolder() 
    {
      //get files from dir into Array
      numTracksInFolder = mp3.getFolderTrackCount(myFolder->folder);
      firstTrack=1;
      //play according to mode
      switch (myFolder->mode)
      {
        case 1:// Hörspielmodus: eine zufällige Datei aus dem Ordner
        {
        Serial.println(F("Hörspielmodus -> zufälligen Track wiedergeben"));
        currentTrack = random(1, numTracksInFolder + 1);
        Serial.println(currentTrack);
        mp3.playFolderTrack(myFolder->folder, currentTrack);
          break;
        }
        case 2:// Album Modus: kompletten Ordner spielen
        {
        Serial.println(F("Album Modus -> kompletten Ordner wiedergeben"));
        currentTrack = 1;
        mp3.playFolderTrack(myFolder->folder, currentTrack);
          break;
        }
        case 3:// Party Modus: Ordner in zufälliger Reihenfolge
        {
        Serial.println(
          F("Party Modus -> Ordner in zufälliger Reihenfolge wiedergeben"));
         shuffleQueue();
        mp3.playFolderTrack(myFolder->folder, queue[0]);
          break;
        }
        case 4:// Einzel Modus: eine Datei aus dem Ordner abspielen
        {
        Serial.println(
          F("Einzel Modus -> eine Datei aus dem Odrdner abspielen"));
        mp3.playFolderTrack(myFolder->folder, myFolder->special);
          break;
        }
        case 5:// Hörbuch Modus: kompletten Ordner spielen und Fortschritt merken
        {
        Serial.println(F("Hörbuch Modus -> kompletten Ordner spielen und "
                         "Fortschritt merken"));
        currentTrack = EEPROM.read(myFolder->folder);
        mp3.playFolderTrack(myFolder->folder, currentTrack);
          break;
        }
        case 6:
        {

          break;
        }
        case 7:// Spezialmodus Von-Bin: Hörspiel: eine zufällige Datei aus dem Ordner
        {
        Serial.println(F("Spezialmodus Von-Bin: Hörspiel -> zufälligen Track wiedergeben"));
        Serial.print(myFolder->special);
        Serial.print(F(" bis "));
        Serial.println(myFolder->special2);
        
        numTracksInFolder = myFolder->special2;
        currentTrack = random(myFolder->special, numTracksInFolder + 1);
        Serial.println(currentTrack, currentTrack);

        mp3.playFolderTrack(myFolder->folder, currentTrack);
          break;
        }
        case 8:// Spezialmodus Von-Bis: Album: alle Dateien zwischen Start und Ende spielen
        {
        Serial.println(F("Spezialmodus Von-Bis: Album: alle Dateien zwischen Start- und Enddatei spielen"));
        Serial.print(myFolder->special);
        Serial.print(F(" bis "));
        Serial.println(myFolder->special2);
        numTracksInFolder = myFolder->special2;
        currentTrack = myFolder->special;
        mp3.playFolderTrack(myFolder->folder, currentTrack);
          break;
        }
        case 9:// Spezialmodus Von-Bis: Party Ordner in zufälliger Reihenfolge
        {
        Serial.println(F("Spezialmodus Von-Bis: Party -> Ordner in zufälliger Reihenfolge wiedergeben"));
        firstTrack = myFolder->special;
        numTracksInFolder = myFolder->special2;
        shuffleQueue();
        currentTrack = 1;
        mp3.playFolderTrack(myFolder->folder, queue[0]);
          break;
        }

        break;
      }
    }

String Player::numToTrack(uint16_t num)
{
  String s_num=String(num);
  while (s_num.length()<3)
    s_num="0"+s_num;
  
  for (byte i=0; i<numTracksInFolder; i++)
  {
    if (listOfTracks[i].startsWith(s_num))
      return listOfTracks[i];
  }
  return "";
}



void playShortCut(uint8_t shortCut) {
  Serial.println(F("=== playShortCut()"));
  Serial.println(shortCut);
  if (mySettings.shortCuts[shortCut].folder != 0) {
    myFolder = &mySettings.shortCuts[shortCut];
    mp3.playFolder();
    disablestandbyTimer();
    delay(1000);
  }
  else
    Serial.println(F("Shortcut not configured!"));
}

bool isPlaying() 
{
  return musicPlayer.playingMusic;
}

void waitForTrackToFinish() {
  long currentTime = millis();
#define TIMEOUT 1000
  do {
    mp3.loop();
  } while (!isPlaying() && millis() < currentTime + TIMEOUT);
  delay(1000);
  do {
    mp3.loop();
  } while (isPlaying());
}

void shuffleQueue() {
  // Queue für die Zufallswiedergabe erstellen
  for (uint8_t x = 0; x < numTracksInFolder - firstTrack + 1; x++)
    queue[x] = x + firstTrack;
  // Rest mit 0 auffüllen
  for (uint8_t x = numTracksInFolder - firstTrack + 1; x < 255; x++)
    queue[x] = 0;
  // Queue mischen
  for (uint8_t i = 0; i < numTracksInFolder - firstTrack + 1; i++)
  {
    uint8_t j = random (0, numTracksInFolder - firstTrack + 1);
    uint8_t t = queue[i];
    queue[i] = queue[j];
    queue[j] = t;
  }
  /*  Serial.println(F("Queue :"));
    for (uint8_t x = 0; x < numTracksInFolder - firstTrack + 1 ; x++)
      Serial.println(queue[x]);
  */
  
}
