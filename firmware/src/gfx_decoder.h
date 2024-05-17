#pragma once
#ifndef GFX_DECODER_H
#define GFX_DECODER_H

#include <Arduino.h>

extern uint16_t gfxFrameLengthsBuffer[12000];
extern uint16_t frameCount;
extern uint8_t gfxFrameBuffer[2048];

bool gfx_decoder_loadNextFrame();

#endif
