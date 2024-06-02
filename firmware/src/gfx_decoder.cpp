#include "gfx_decoder.h"
#include "sd.h"
#include "lodepng.h"
#include "leds.h"

uint16_t gfxFrameLengthsBuffer[24000] = {0};
uint16_t frameCount = 0;

uint8_t gfxFrameBuffer[6400] = {0};

int32_t gfx_decoder_loadNextFrame() {
  // load frame from SD card
  auto frameSize = sd_loadNextFrame();
  if (frameSize < 0) {
    return frameSize;
  }

  // decode PNG
  unsigned error;
  unsigned char *buffer = 0;
  unsigned width, height;

  size_t pngSize = frameSize;
  error = lodepng_decode_memory(&buffer, &width, &height, gfxFrameBuffer, pngSize, LCT_GREY, 8);

  // handle errors
  if (error) {
    Serial.print("PNG decode error ");
    Serial.print(error);
    Serial.print(": ");
    Serial.println(lodepng_error_text(error));
    free(buffer);
    return false;
  } else if (width != ROW_COUNT || height != COL_COUNT) {
    Serial.print("Bad dimensions: ");
    Serial.print(width);
    Serial.print("x");
    Serial.println(height);
    free(buffer);
    return false;
  }

  leds_set_framebuffer(buffer);

  free(buffer);
  return frameSize;
}

unsigned long frameLastChangedAt = 0;

int32_t gfx_decoder_handleLoop() {
  auto now = millis();
  if (now - frameLastChangedAt > MS_PER_FRAME) {
    frameLastChangedAt = now;
    return gfx_decoder_loadNextFrame();
  }
  return 0;
}

void gfx_decoder_setTestFrame() {
  uint8_t buffer[ROW_COUNT * COL_COUNT] = {0};

  // le boxes
  for (int i = 0; i < 8; i++) {
    uint8_t color = 1 << i;
    int startX = (i % 4) * 10;
    int startY = (i / 4) * 10;

    // box with only 1<<n color
    for (int x = startX; x < startX + 10; x++) {
      for (int y = startY + 5; y < startY + 10; y++) {
        buffer[y * ROW_COUNT + x] = color;
      }
    }

    // box with 1<<n - 1 color for comparison
    for (int x = startX; x < startX + 10; x++) {
      for (int y = startY; y < startY + 5; y++) {
        buffer[y * ROW_COUNT + x] = color - 1;
      }
    }
  }

  // full color
  for (int x = 30; x < ROW_COUNT; x++) {
    for (int y = 20; y < 25; y++) {
      buffer[y * ROW_COUNT + x] = 255;
    }
  }

  // smooth gradient - lower range
  for (int x = 0; x < ROW_COUNT; x++) {
    for (int y = 30; y < 35; y++) {
      buffer[y * ROW_COUNT + x] = x;
    }
  }

  // smooth gradient
  float delta = 256 / (COL_COUNT);
  for (int x = 0; x < ROW_COUNT; x++) {
    for (int y = 35; y < 40; y++) {
      buffer[y * ROW_COUNT + x] = (x + 1) * delta;
    }
  }

  leds_set_framebuffer(buffer);
}
