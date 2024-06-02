#pragma once
#ifndef GFX_DECODER_H
#define GFX_DECODER_H

#include <Arduino.h>

extern uint16_t gfxFrameLengthsBuffer[24000];
extern uint16_t frameCount;
extern uint8_t gfxFrameBuffer[6400];

int32_t gfx_decoder_loadNextFrame();
int32_t gfx_decoder_handleLoop();
void gfx_decoder_setTestFrame();

#endif
