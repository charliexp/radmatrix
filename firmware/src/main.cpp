#include <Arduino.h>
#include "hardware/gpio.h"
#include "audio.h"
#include "sd.h"
#include "leds.h"
#include "gfx_decoder.h"
#include "can2040.h"

void loadVideo(size_t index);

void setup() {
  leds_init();
  setupSDPins();
  pinMode(4, INPUT_PULLUP);

  delay(2000);
  Serial.begin(115200);
  Serial.println("Hello worldd!");

  init_audio();
  leds_initRenderer();

  // while (!isSDCardInserted()) {
  //   Serial.println("SD card not connected, waiting...");
  //   delay(1000);
  // }
  // delay(100);

  setupSD();
  sd_loadPlaylist();

  loadVideo(0);

  // gfx_decoder_setTestFrame();
}

size_t currentVideoIndex = 0;
bool isLoaded = false;

void loadVideo(size_t index) {
  audio_stop();

  sd_loadAudio(index);

  if (!sd_loadGfxFrameLengths(index)) {
    Serial.println("Failed to load gfx frame lengths");
    while (true) {}
  }

  if (!sd_loadGfxBlob(index)) {
    Serial.println("Failed to load gfx blob");
    while (true) {}
  }

  isLoaded = true;
}

void nextSong() {
  Serial.println("Next song!");
  currentVideoIndex = (currentVideoIndex + 1) % playlistSize;
  loadVideo(currentVideoIndex);
}

void loop() {
  if (digitalRead(4) == LOW) {
    nextSong();
  }

  // if (Serial.available() > 0) {
  //   char c = Serial.read();
  //   if (c == 'p') {
  //     Serial.println("Paused. Press any key to continue.");
  //     leds_disable();
  //     while (Serial.available() == 0) {
  //       Serial.read();
  //       delay(50);
  //     }
  //     Serial.println("Continuing...");
  //   }
  // }

  if (isLoaded) {
    sd_loadNextAudio();

    auto loopStatus = gfx_decoder_handleLoop();

    if (loopStatus == -1) {
      Serial.println("Failed to load frame...");
    } else if (loopStatus == -2) {
      nextSong();
    }
  }
}
