#include <Arduino.h>
#include "config.h"

#define CPU_MHZ ((uint32_t) CPU_CLOCK_HZ / 1000000)
#define NS_PER_CYCLE (1000 / CPU_MHZ)
#define NS_TO_DELAY(ns) (ns / NS_PER_CYCLE / LEDS_PIO_CLKDIV)

// delays in nanoseconds per brightness phase (i.e. depth bit index)
uint32_t brightnessPhaseDelays[COLOR_BITS] = {
  // NOTE: 100ns seems to be the minimum that's (barely) visible on current hardware
  /*   1 */ NS_TO_DELAY(170),
  /*   2 */ NS_TO_DELAY(180),
  /*   4 */ NS_TO_DELAY(210),
  /*   8 */ NS_TO_DELAY(1740),
  /*  16 */ NS_TO_DELAY(2100), // x2
  /*  32 */ NS_TO_DELAY(3000), // x4
  /*  64 */ NS_TO_DELAY(2500), // x10
  /* 128 */ NS_TO_DELAY(3300), // x20
};

uint8_t brightnessPhaseDithering[COLOR_BITS] = {
  // Out of DITHERING_PHASES, how many of these should a given
  // brightness phase be displayed?
  // NOTE: This is done brecause for small delays, pixel pushing dominates the time, making
  // the display's duty cycle (and hence brightness) low. But since these less significant bits
  // contribute little to the overall brightness, and overall displaying time is short (a fraction of
  // a framerate), we can skip displaying these small brightness levels most of the time.
  /*   1 */ 1,
  /*   2 */ 1,
  /*   4 */ 1,
  /*   8 */ 1,
  /*  16 */ 2,
  /*  32 */ 4,
  /*  64 */ 10,
  /* 128 */ 20,
};
