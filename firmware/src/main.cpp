#include <Arduino.h>
#include "hardware/gpio.h"
#include "audio.h"
#include "sd.h"
#include "leds.h"
#include "gfx_decoder.h"

void setup() {
  leds_init();
  setupSDPins();

  delay(2000);
  Serial.begin(115200);
  Serial.println("Hello worldd!");

  init_audio();

  // while (!isSDCardInserted()) {
  //   Serial.println("SD card not connected, waiting...");
  //   delay(1000);
  // }
  // delay(100);

  setupSD();

  // // sd_getAudio();
  // sd_getGfx();

  // return;

  if (!sd_loadGfxFrameLengths()) {
    Serial.println("Failed to load gfx frame lengths");
    while (true) {}
  }

  if (!sd_loadGfxBlob()) {
    Serial.println("Failed to load gfx blob");
    while (true) {}
  }
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'p') {
      Serial.println("Paused. Press any key to continue.");
      leds_disable();
      while (Serial.available() == 0) {
        Serial.read();
        delay(50);
      }
      Serial.println("Continuing...");
    }
  }


  if (!gfx_decoder_loadNextFrame()) {
    Serial.println("Failed to load frame...");
  }

  leds_render();
  leds_render();
  leds_render();
  leds_render();
}
