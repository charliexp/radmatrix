#pragma once
#ifndef GFX_DECODER_H
#define GFX_DECODER_H

#include <Arduino.h>
#include "config.h"

#define GFX_DECODER_LENGTH_BUFFER_SIZE MAX_LENGTH_MIN * 60 * MAX_FPS
#define GFX_DECODER_FRAME_BUFFER_SIZE ROW_COUNT * COL_COUNT * 4

extern uint16_t gfxFrameLengthsBuffer[GFX_DECODER_LENGTH_BUFFER_SIZE];
extern uint16_t frameCount;
extern uint8_t gfxFrameBuffer[GFX_DECODER_FRAME_BUFFER_SIZE];

int32_t gfx_decoder_loadNextFrame();
int32_t gfx_decoder_handleLoop();
void gfx_decoder_setTestFrame();

#endif
