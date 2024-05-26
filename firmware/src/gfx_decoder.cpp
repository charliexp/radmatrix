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

  // Convert framebuffer into raw shift register data for fast PIO pixel pushing
  // Data will be held in buffers, one per pixel's depth bit (aka brightness stage),
  // with each row split into 32-bit chunks, one per module
  // (20 pixels, 24 shift register stages, 8 unused bits)
  // TODO: Move this to leds.cpp
  // TODO: Use a separate buffer, then copy to ledsBuffer to avoid tearing
  for (int bi = 0; bi < 8; bi++) {
    uint8_t bitPosition = 1 << bi;
    for (int y = 0; y < ROW_COUNT; y++) {
      auto yOffset = y * COL_COUNT;
      for (int xModule = 0; xModule < COL_MODULES; xModule++) {
        auto bufferXOffset = yOffset + xModule * 20;
        uint32_t sample = 0;

        for (int x = 0; x < 20; x++) {
          // insert placeholders for unused stages
          // (before pixels 0, 6, 13)
          if (x == 0 || x == 6 || x == 13) {
            sample >>= 1;
          }
          uint8_t px = buffer[bufferXOffset + x];
          bool bit = px & bitPosition;
          sample = (sample >> 1) | (bit ? 0x80000000 : 0);
        }
        // insert placeholder for unused last stage (after pixel 19)
        sample >>=1;

        ledBuffer[bi][y * COL_MODULES + xModule] = sample;
      }
    }
  }

  // copy to framebuffer
  // TODO: mutex? double buffer? or something...
  memcpy(framebuffer, buffer, ROW_COUNT * COL_COUNT);
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
