#pragma once

#ifndef _config_h
#define _config_h

// --- pinout ---

#define AUDIO_PIN 8

#define SD_PIN_SS 1
#define SD_PIN_SCK 2
#define SD_PIN_MOSI 3
#define SD_PIN_MISO 0
#define SD_HAS_DETECTION true
#define SD_DET_PIN 4

#define CAN_ENABLED false
#define CAN_PIN_RX 5
#define CAN_PIN_TX 7
#define CAN_PIN_SILENT 6

#define UART_TX_PIN 16
#define UART_RX_PIN 17

// NOTE: with current layout, 9-15 are free GPIOs (14, 15 only when using shared SER, SRCLR)
#define NEXT_PIN 9

#define CC1_PIN 26
#define CC2_PIN 27
#define V_SENSE_PIN 28
#define I_SENSE_PIN 29

// --- pinout - screen ---

// #define COMMON_SER 21
// #define COMMON_RCLK 23
#define COMMON_SRCLR 25

#define COL_SER 21
#define COL_OE 22
#define COL_RCLK 23
#define COL_SRCLK 24
// #define COL_SRCLR 25

#define ROW_SER 14
#define ROW_OE 19
#define ROW_RCLK 20
#define ROW_SRCLK 18
// #define ROW_SRCLR 15

#define COL_SER_INVERTED true
#define COL_OE_INVERTED true
#define COL_RCLK_INVERTED true
#define COL_SRCLK_INVERTED true
#define COL_SRCLR_INVERTED false

// --- screen settings ---

#define ROW_MODULES 2
#define COL_MODULES 2

#define COLOR_BITS 8

#define MAX_FPS 30
#define MAX_LENGTH_MIN 10

#define ROW_COUNT ROW_MODULES * 20
#define COL_COUNT COL_MODULES * 20

// --- audio settings ---

#define AUDIO_RATE 44000.0f
#define BUFFER_LEN 512*16

// --- other settings ---

#define CPU_CLOCK_HZ 125000000.0f
#define SD_CARD_BAUD_RATE 12 * 1000 * 1000
#define REFERENCE_VOLTAGE 3.3f // for ADC
#define CAN_BITRATE 500000

// --- debug settings ---

#define DEBUG_TEST_FRAME false
#define DEBUG_FIRST_FRAME false
#define DEBUG_FRAMEBUFFER false

// --- screen - performance ---
// NOTE: In case of screen glitching, these may need to be tweaked to decrease
// data rate to the display and stay within the limits of the hardware/electrical connection
#define LEDS_PIO_CLKDIV 1
// Also see leds.pio where delays may be adjusted (need manual compilation)

// --- screen - color correction ---
// I do not understand color correction, gamma, it's all black magic to me
// This was manually tuned using DEBUG_TEST_FRAME=true test pattern
// See config.cpp
extern uint32_t brightnessPhaseDelays[COLOR_BITS];
extern uint8_t brightnessPhaseDithering[COLOR_BITS];
#define DITHERING_PHASES 20;

#endif
