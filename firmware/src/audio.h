#pragma once

#ifndef _audio_h
#define _audio_h

#define AUDIO_PIN 8

#define AUDIO_RATE 44000.0f
#define BUFFER_LEN 512*16
#define BUFFER_LEN_MS (BUFFER_LEN / AUDIO_RATE) * 1000.0f

extern uint8_t wav_buffer_0[BUFFER_LEN];
extern uint8_t wav_buffer_1[BUFFER_LEN];
extern bool wav_buffer1_active;
extern volatile bool next_buffer_requested;

void init_audio();
void audio_stop();
void audio_start();

#endif
