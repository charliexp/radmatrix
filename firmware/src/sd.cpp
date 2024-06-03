#include <Arduino.h>
#include <SPI.h>
#include "sd.h"
#include "audio.h"
#include "gfx_decoder.h"

#include "hw_config.h"
#include "ff.h"
#include "f_util.h"

#define SD_DET_PIN 4

#define SD_PIN_SS 1
#define SD_PIN_SCK 2
#define SD_PIN_MOSI 3
#define SD_PIN_MISO 0

static spi_t spi = {
  .hw_inst = spi0,
  .miso_gpio = SD_PIN_MISO,
  .mosi_gpio = SD_PIN_MOSI,
  .sck_gpio = SD_PIN_SCK,
  .baud_rate = 10 * 1000 * 1000,
};

static sd_spi_if_t spi_if = {
  .spi = &spi,
  .ss_gpio = SD_PIN_SS,
};

static sd_card_t sd_card = {
  .type = SD_IF_SPI,
  .spi_if_p = &spi_if,
};

size_t sd_get_num() {
  return 1;
}

sd_card_t *sd_get_by_num(size_t num) {
  if (num == 0) {
    return &sd_card;
  }

  return nullptr;
}

void setupSDPins() {
  // TODO: Is that even needed if we use built-in SPI?
}

bool isSDCardInserted() {
  return digitalRead(SD_DET_PIN) == LOW;
}

#define CHECK_RESULT(result, caller) \
  if (result != FR_OK) { \
    Serial.print(caller); \
    Serial.print(" error: "); \
    Serial.print(FRESULT_str(result)); \
    Serial.print(" ("); \
    Serial.print(result); \
    Serial.println(")"); \
  }

FATFS *fs;

void setupSD() {
  Serial.println("Initializing SD card...");

  sd_init_driver();

  fs = (FATFS*) malloc(sizeof(FATFS));
  FRESULT result = f_mount(fs, "", 1);
  CHECK_RESULT(result, "f_mount");

  Serial.println("SD Initialization done");
}

String playlist[128] = {};
size_t playlistSize = 0;

void sd_loadPlaylist() {
  auto path = "video/playlist.txt";

  FIL playlistFile;
  FRESULT result = f_open(&playlistFile, path, FA_READ);
  CHECK_RESULT(result, "playlist file open");

  Serial.println("Playlist file opened");

  char playlist_buffer[512];
  auto fileSize = f_size(&playlistFile);

  if (fileSize > sizeof(playlist_buffer)) {
    Serial.print("Playlist file too large, max: ");
    Serial.println(sizeof(playlist_buffer));
    return;
  }

  unsigned int bytesRead;
  result = f_read(&playlistFile, &playlist_buffer, fileSize, &bytesRead);
  CHECK_RESULT(result, "playlist file read");

  if (bytesRead != fileSize) {
    Serial.print("playlist file read error: read ");
    Serial.print(bytesRead);
    Serial.print(" bytes, expected ");
    Serial.println(fileSize);
    return;
  }

  result = f_close(&playlistFile);
  CHECK_RESULT(result, "playlist file close");

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
}

FIL *audioFile;

void sd_loadAudio(size_t index) {
  if (index >= playlistSize) {
    Serial.println("Index out of range");
    return;
  }

  auto path = "video/" + playlist[index] + "/audio.bin";

  FRESULT result;

  if (audioFile) {
    result = f_close(audioFile);
    CHECK_RESULT(result, "audio file close");
    free(audioFile);
  }

  audioFile = (FIL*) malloc(sizeof(FIL));
  result = f_open(audioFile, path.c_str(), FA_READ);
  CHECK_RESULT(result, "audio file open");

  Serial.println("Audio file opened");

  audio_stop();

  // load two buffers' worth of audio
  unsigned int bytesRead;
  result = f_read(audioFile, &wav_buffer_0, BUFFER_LEN, &bytesRead);
  CHECK_RESULT(result, "first audio sample");
  if (bytesRead < BUFFER_LEN) {
    Serial.println("Could not read first sample");
    return;
  }

  result = f_read(audioFile, &wav_buffer_1, BUFFER_LEN, &bytesRead);
  CHECK_RESULT(result, "second audio sample");
  if (bytesRead < BUFFER_LEN) {
    Serial.println("Could not read second sample");
    return;
  }

  audio_start();
}

void sd_loadNextAudio() {
  if (!next_buffer_requested) {
    return;
  }
  next_buffer_requested = false;

  auto b4 = millis();
  auto next_buffer = wav_buffer1_active ? &wav_buffer_0 : &wav_buffer_1;

  FRESULT result;
  unsigned int bytesRead;

  result = f_read(audioFile, next_buffer, BUFFER_LEN, &bytesRead);
  CHECK_RESULT(result, "audio sample");

  if (bytesRead < BUFFER_LEN) {
    Serial.println("End of audio.");
    audio_stop();
  } else {
    /*
    Serial.print("Read ");
    Serial.print(bytesRead);
    Serial.print(" bytes from audio file in ");
    Serial.print(millis() - b4);
    Serial.println("ms");
    */
  }
}

bool sd_loadGfxFrameLengths(size_t index) {
  if (index >= playlistSize) {
    Serial.println("Index out of range");
    return false;
  }

  auto path = "video/" + playlist[index] + "/gfx_len.bin";

  FIL lengthsFile;
  FRESULT result = f_open(&lengthsFile, path.c_str(), FA_READ);
  CHECK_RESULT(result, "frame lengths file open");

  auto fileSize = f_size(&lengthsFile);

  if (fileSize > sizeof(gfxFrameLengthsBuffer)) {
    Serial.println("Frame lengths file too large");
    return false;
  }
  Serial.println(fileSize);
  frameCount = fileSize / sizeof(uint16_t);

  unsigned int bytesRead;
  result = f_read(&lengthsFile, &gfxFrameLengthsBuffer, fileSize, &bytesRead);
  CHECK_RESULT(result, "playlist file read");

  if (bytesRead != fileSize) {
    Serial.print("frame lengths file read error: read ");
    Serial.print(bytesRead);
    Serial.print(" bytes, expected ");
    Serial.println(fileSize);
    return false;
  }

  result = f_close(&lengthsFile);
  CHECK_RESULT(result, "frame lengths file close");

  return true;
}

FIL *gfxFile;

uint16_t frameIdx = 0;

bool sd_loadGfxBlob(size_t index) {
  if (index >= playlistSize) {
    Serial.println("Index out of range");
    return false;
  }

  auto path = "video/" + playlist[index] + "/gfx.bin";

  FRESULT result;

  if (gfxFile) {
    result = f_close(gfxFile);
    CHECK_RESULT(result, "gfx file close");
    free(gfxFile);
  }

  gfxFile = (FIL*) malloc(sizeof(FIL));
  result = f_open(gfxFile, path.c_str(), FA_READ);
  CHECK_RESULT(result, "gfx blob file open");

  Serial.println("Opened video frames");

  frameIdx = 0;

  return true;
}

// Returns size of frame read or -1 if error
int32_t sd_loadNextFrame() {
  if (frameIdx > 0) {
    // return -1;
  }

  if (!gfxFile) {
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
    while (true) {}
    return -1;
  }

  // read data
  unsigned int bytesRead;
  FRESULT result = f_read(gfxFile, &gfxFrameBuffer, frameSize, &bytesRead);
  CHECK_RESULT(result, "playlist file read");

  if (bytesRead < frameSize) {
    Serial.println("Could not read the entire frame");
    return -1;
  }

  // increment
  if (frameIdx == frameCount - 1) {
    Serial.println("Last frame, next video!");
    return -2;
  } else {
    frameIdx++;
  }

  return frameSize;
}
