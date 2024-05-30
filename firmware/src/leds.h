#pragma once
#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

#define COL_SER 20
#define COL_OE 21
// #define COL_RCLK 22
#define RCLK 22
#define COL_SRCLK 26
#define COL_SRCLR 27

#define ROW_SER 14
#define ROW_OE 13
// #define ROW_RCLK 12
#define ROW_SRCLK 11
#define ROW_SRCLR 10

#define ROW_MODULES 2
#define ROW_COUNT ROW_MODULES * 20
#define COL_MODULES 2
#define COL_COUNT COL_MODULES * 20

#define COLOR_BITS 5
#define FPS 30
#define MS_PER_FRAME 1000 / FPS

void leds_init();
void leds_initRenderer();
void leds_disable();
void leds_loop();
void leds_render();

void leds_set_framebuffer(uint8_t *buffer);

#endif
