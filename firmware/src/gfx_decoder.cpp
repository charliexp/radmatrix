#include "gfx_decoder.h"
#include "sd.h"
#include "lodepng.h"
#include "leds.h"

uint16_t gfxFrameLengthsBuffer[12000] = {0};
uint16_t frameCount = 0;

uint8_t gfxFrameBuffer[6400] = {0};

bool gfx_decoder_loadNextFrame() {
  // load frame from SD card
  auto frameSize = sd_loadNextFrame();
  if (frameSize < 0) {
    return false;
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

  // copy to framebuffer
  // TODO: mutex? double buffer? or something...
  memcpy(framebuffer, buffer, ROW_COUNT * COL_COUNT);
  free(buffer);
  return true;
}

unsigned long frameLastChangedAt = 0;

bool gfx_decoder_handleLoop() {
  auto now = millis();
  if (now - frameLastChangedAt > MS_PER_FRAME) {
    frameLastChangedAt = now;
    return gfx_decoder_loadNextFrame();
  }
  return true;
}
