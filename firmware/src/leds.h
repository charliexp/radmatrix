#pragma once
#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

#define COMMON_SER 21
// #define COMMON_RCLK 23
#define COMMON_SRCLR 25

// #define COL_SER 21
#define COL_OE 22
#define COL_RCLK 23
#define COL_SRCLK 24
// #define COL_SRCLR 25

// #define ROW_SER 14
#define ROW_OE 13
#define ROW_RCLK 20
#define ROW_SRCLK 18
// #define ROW_SRCLR 15

#if defined(COMMON_SER) && (defined(ROW_SER) || defined(COL_SER))
#error "COMMON_SER and ROW_SER/COL_SER cannot be defined at the same time"
#endif

#if defined(COMMON_SER)
#define COL_SER COMMON_SER
#define ROW_SER COMMON_SER
#endif

#if defined(COMMON_RCLK) && (defined(ROW_RCLK) || defined(COL_RCLK))
#error "COMMON_RCLK and ROW_RCLK/COL_RCLK cannot be defined at the same time"
#endif

#if defined(COMMON_RCLK)
#define COL_RCLK COMMON_RCLK
#define ROW_RCLK COMMON_RCLK
#endif

#if defined(COMMON_SRCLR) && (defined(ROW_SRCLR) || defined(COL_SRCLR))
#error "COMMON_SRCLR and ROW_SRCLR/COL_SRCLR cannot be defined at the same time"
#endif

#if defined(COMMON_SRCLR)
#define COL_SRCLR COMMON_SRCLR
#define ROW_SRCLR COMMON_SRCLR
#endif

#define ROW_MODULES 2
#define ROW_COUNT ROW_MODULES * 20
#define COL_MODULES 2
#define COL_COUNT COL_MODULES * 20

#define COLOR_BITS 8
#define FPS 30
#define MS_PER_FRAME (1000 / FPS)

#define CPU_MHZ 125
#define NS_PER_CYCLE (1000 / CPU_MHZ)

void leds_init();
void leds_initRenderer();
void leds_disable();
void leds_loop();
void leds_render();

void leds_set_framebuffer(uint8_t *buffer);

#endif
