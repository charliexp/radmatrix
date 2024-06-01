#include <Arduino.h>
#include <SPI.h>
// #include <RP2040_SD.h>
#include "sd.h"
#include "audio.h"
#include "gfx_decoder.h"

#include "hw_config.h"
#include "ff.h"
#include "f_util.h"

#define SD_DET_PIN 28

static spi_t spi = {
  .hw_inst = spi0,
  .miso_gpio = 4,
  .mosi_gpio = 3,
  .sck_gpio = 2,
  .baud_rate = 10 * 1000 * 1000,
};

static sd_spi_if_t spi_if = {
  .spi = &spi,
  .ss_gpio = 7,
};

static sd_card_t sd_card = {
  .type = SD_IF_SPI,
  .spi_if_p = &spi_if,
};

void sd_test() {
  FATFS fs;
  FRESULT fr = f_mount(&fs, "", 1);
  if (FR_OK != fr) panic("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
  FIL fil;
  const char* const filename = "filename.txt";
  fr = f_open(&fil, filename, FA_OPEN_APPEND | FA_WRITE);
  if (FR_OK != fr && FR_EXIST != fr)
      panic("f_open(%s) error: %s (%d)\n", filename, FRESULT_str(fr), fr);
  if (f_printf(&fil, "Hello, world!\n") < 0) {
      printf("f_printf failed\n");
  }
  fr = f_close(&fil);
  if (FR_OK != fr) {
      printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
  }
  f_unmount("");
}

/*
#if PIN_SPI_SS != 17
#error "PIN_SPI_SS must be 17"
#endif
#if PIN_SPI_SCK != 18
#error "PIN_SPI_SCK must be 18"
#endif
#if PIN_SPI_MOSI != 19
#error "PIN_SPI_MOSI must be 19"
#endif
#if PIN_SPI_MISO != 16
#error "PIN_SPI_MISO must be 16"
#endif
*/

void setupSDPins() {
  // TODO: Is that even needed if we use built-in SPI?
  /*
  pinMode(PIN_SPI_MISO, INPUT);
  pinMode(PIN_SPI_SS, OUTPUT);
  digitalWrite(PIN_SPI_SS, HIGH);
  pinMode(PIN_SPI_SCK, OUTPUT);
  pinMode(PIN_SPI_MOSI, OUTPUT);
  pinMode(SD_DET_PIN, INPUT_PULLUP);
  */
}

bool isSDCardInserted() {
  return digitalRead(SD_DET_PIN) == LOW;
}

/*
void printSDConfig();
void testSDCard();
void printSDStats();
void printDirectory(File dir, int numTabs);
*/

void setupSD() {
  /*
  SPI.begin();

  // printSDConfig();

  if (!SD.begin(20000000, PIN_SPI_SS)) {
    Serial.println("SD Initialization failed!");
    // Serial.print("Error code: ");
    // Serial.println(SD.card.errorCode(), HEX);
    while (true) {}
    return;
  }

  Serial.println("SD Initialization done");

  // testSDCard();
  // printSDStats(SD.volume);

  // File root = SD.open("/");
  // printDirectory(root, 0);
  */
}

/*
void printSDConfig() {
  Serial.println(BOARD_NAME);
  Serial.println(RP2040_SD_VERSION);

  Serial.print("Initializing SD card with SS = ");
  Serial.println(PIN_SPI_SS);
  Serial.print("SCK = ");
  Serial.println(PIN_SPI_SCK);
  Serial.print("MOSI = ");
  Serial.println(PIN_SPI_MOSI);
  Serial.print("MISO = ");
  Serial.println(PIN_SPI_MISO);
}

void testSDCard() {
  File myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    myFile.close();
    Serial.println("done.");
  } else {
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    myFile.close();
  } else {
    Serial.println("error opening test.txt");
  }
}

void printDirectory(File dir, int numTabs) {
  while (true) {
    File entry = dir.openNextFile();

    if (!entry) {
      // no more files
      break;
    }

    for (uint8_t i = 0; i < numTabs; i++) {
      Serial.print('\t');
    }

    Serial.print(entry.name());

    if (entry.isDirectory()) {
      Serial.println("/");
      printDirectory(entry, numTabs + 1);
    } else {
      // files have sizes, directories do not
      Serial.print("\t\t");
      Serial.println(entry.size(), DEC);
    }
    entry.close();
  }
}

void printSDStats(RP2040_SdVolume volume) {
  Serial.print("Clusters:          ");
  Serial.println(volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(volume.blocksPerCluster() * volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(volume.fatType(), DEC);

  volumesize = volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1 KB)
  Serial.print("Volume size (KB):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (MB):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (GB):  ");
  Serial.println((float)volumesize / 1024.0);
}
*/
String playlist[128] = {};
size_t playlistSize = 0;

void sd_loadPlaylist() {
  /*
  auto path = "video/playlist.txt";

  if (!SD.exists(path)) {
    Serial.println("Could not find playlist for videos :(");
    return;
  }

  auto playlistFile = SD.open(path, FILE_READ);
  Serial.println("Playlist file opened");

  char playlist_buffer[512];
  auto fileSize = playlistFile.size();
  if (fileSize > sizeof(playlist_buffer)) {
    Serial.print("Playlist file too large, max: ");
    Serial.println(sizeof(playlist_buffer));
    return;
  }

  if (playlistFile.read(&playlist_buffer, sizeof(playlist_buffer)) != fileSize) {
    Serial.println("Could not read playlist file");
    return;
  }

  playlistFile.close();

  Serial.println("Parsing playlist...");

  // parse playlist
  auto playlistStr = String(playlist_buffer, fileSize);
  Serial.println(playlistStr);

  auto idx = 0;
  while (true) {
    auto nextIdx = playlistStr.indexOf('\n', idx);
    if (nextIdx == -1) {
      break;
    }

    auto line = playlistStr.substring(idx, nextIdx);
    if (line.length() == 0) {
      break;
    }
    if (line.length() > 8) {
      Serial.print("Video name too long, size: ");
      Serial.print(line.length());
      Serial.println(", max 8");
      break;
    }

    playlist[playlistSize++] = line;
    idx = nextIdx + 1;
  }

  Serial.println("Playlist loaded");

  for (size_t i = 0; i < playlistSize; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.println(playlist[i]);
  }
  */
}

// File audioFile;

void sd_loadAudio(size_t index) {
  /*
  if (index >= playlistSize) {
    Serial.println("Index out of range");
    return;
  }

  auto path = "video/" + playlist[index] + "/audio.bin";

  if (!SD.exists(path)) {
    Serial.println("Audio not found :(");
    return;
  }

  if (audioFile) {
    audioFile.close();
  }

  audioFile = SD.open(path, FILE_READ);
  Serial.println("Audio file opened");

  audio_stop();

  // load two buffers' worth of audio
  if (audioFile.read(&wav_buffer_0, BUFFER_LEN) < BUFFER_LEN) {
    Serial.println("Could not read first sample");
    return;
  }

  if (audioFile.read(&wav_buffer_1, BUFFER_LEN) < BUFFER_LEN) {
    Serial.println("Could not read second sample");
    return;
  }

  audio_start();
  */
}

void sd_loadNextAudio() {
  /*
  if (!next_buffer_requested) {
    return;
  }
  next_buffer_requested = false;

  auto b4 = millis();
  auto next_buffer = wav_buffer1_active ? &wav_buffer_0 : &wav_buffer_1;
  auto bytesRead = audioFile.read(next_buffer, BUFFER_LEN);

  if (bytesRead < BUFFER_LEN) {
    // Serial.println("End of audio file, rewinding...");
    // audioFile.seek(0);
    Serial.println("End of audio.");
    audio_stop();
  } else {
    /*
    Serial.print("Read ");
    Serial.print(bytesRead);
    Serial.print(" bytes from audio file in ");
    Serial.print(millis() - b4);
    Serial.println("ms");
    * /
  }
  */
}

bool sd_loadGfxFrameLengths(size_t index) {
  return false;
  /*
  if (index >= playlistSize) {
    Serial.println("Index out of range");
    return false;
  }

  auto path = "video/" + playlist[index] + "/gfx_len.bin";

  if (!SD.exists(path)) {
    Serial.println("Frame lengths file not found :(");
    return false;
  }

  auto lengthsFile = SD.open(path, FILE_READ);
  auto fileSize = lengthsFile.size();

  if (fileSize > sizeof(gfxFrameLengthsBuffer)) {
    Serial.println("Frame lengths file too large");
    return false;
  }

  frameCount = fileSize / sizeof(uint16_t);

  while (lengthsFile.available()) {
    lengthsFile.read(&gfxFrameLengthsBuffer, sizeof(gfxFrameLengthsBuffer));
  }

  lengthsFile.close();
  Serial.println("Done reading frame lengths");

  return true;
  */
}

// File gfxFile;

uint16_t frameIdx = 0;

bool sd_loadGfxBlob(size_t index) {
  return false;
  /*
  if (index >= playlistSize) {
    Serial.println("Index out of range");
    return false;
  }

  auto path = "video/" + playlist[index] + "/gfx.bin";

  if (!SD.exists(path)) {
    Serial.println("Gfx blob file not found :(");
    return false;
  }

  gfxFile = SD.open(path, FILE_READ);
  Serial.println("Opened video frames");

  frameIdx = 0;

  return true;
  */
}

// Returns size of frame read or -1 if error
int32_t sd_loadNextFrame() {
  return -1;
  /*
  if (frameIdx > 0) {
    // return -1;
  }
  if (!gfxFile || !gfxFile.available()) {
    Serial.println("Gfx file not available");
    return -1;
  }

  if (frameIdx >= frameCount) {
    Serial.println("Frame out of range");
    return -1;
  }

  // get size of frame png
  auto frameSize = gfxFrameLengthsBuffer[frameIdx];
  if (frameSize > sizeof(gfxFrameBuffer)) {
    Serial.print("Frame too large: ");
    Serial.println(frameSize);
    return -1;
  }

  // read data
  auto bytesRead = gfxFile.read(&gfxFrameBuffer, frameSize);
  if (bytesRead < frameSize) {
    Serial.println("Could not read the entire frame");
    return -1;
  }

  // increment
  if (frameIdx == frameCount - 1) {
    // Serial.println("Last frame, rewinding...");
    // gfxFile.seek(0);
    // frameIdx = 0;
    Serial.println("Last frame, next video!");
    return -2;
  } else {
    frameIdx++;
  }

  return frameSize;
  */
}
