#pragma once

#ifndef _audio_h
#define _audio_h

#include "config.h"

extern uint8_t wav_buffer_0[BUFFER_LEN];
extern uint8_t wav_buffer_1[BUFFER_LEN];
extern bool wav_buffer1_active;
extern volatile bool next_buffer_requested;

void init_audio();
void audio_stop();
void audio_start();

#endif
