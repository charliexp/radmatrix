#include <Arduino.h>
#include "hardware/gpio.h"
#include "mbed_wait_api.h"
#include "pico/multicore.h"

#include "leds.h"

inline void pulsePin(uint8_t pin) {
  gpio_put(pin, HIGH);
  gpio_put(pin, LOW);
}

void clearShiftReg(uint8_t srclk, uint8_t srclr) {
  gpio_put(srclr, LOW);
  pulsePin(srclk);
  gpio_put(srclr, HIGH);
}

inline void outputEnable(uint8_t pin, bool enable) {
  gpio_put(pin, !enable);
}

// we have 4-bit color depth, so 16 levels of brightness
// we go from phase 0 to phase 3
uint8_t brightnessPhase = 0;
uint8_t brightnessPhaseDelays[] = {1, 10, 30, 100};

uint8_t framebuffer[ROW_COUNT * COL_COUNT] = {0};

void leds_init() {
  memset(framebuffer, 0, sizeof(framebuffer));

  // disable output
  outputEnable(COL_OE, false);
  outputEnable(ROW_OE, false);

  // set up col pins
  pinMode(COL_SER, OUTPUT);
  pinMode(COL_OE, OUTPUT);
  pinMode(COL_RCLK, OUTPUT);
  pinMode(COL_SRCLK, OUTPUT);
  pinMode(COL_SRCLR, OUTPUT);

  // set up row pins
  pinMode(ROW_SER, OUTPUT);
  pinMode(ROW_OE, OUTPUT);
  pinMode(ROW_RCLK, OUTPUT);
  pinMode(ROW_SRCLK, OUTPUT);
  pinMode(ROW_SRCLR, OUTPUT);

  // clear output - cols
  clearShiftReg(COL_SRCLK, COL_SRCLR);
  pulsePin(COL_RCLK);
  outputEnable(COL_OE, true);

  // clear output - rows
  clearShiftReg(ROW_SRCLK, ROW_SRCLR);
  pulsePin(ROW_RCLK);
}

void leds_disable() {
  outputEnable(ROW_OE, false);
}

void main2() {
  // where we're going, we don't need no interrupts
  noInterrupts();
  while (true) {
    leds_render();
  }
}

void leds_initRenderer() {
  // launch core1
  // NOTE: For some reason, without delay, core1 doesn't start?
  // delay(500);
  multicore_reset_core1();
  multicore_launch_core1(main2);
}

void leds_render() {
  // hide output
  outputEnable(ROW_OE, false);

  // clear rows
  clearShiftReg(ROW_SRCLK, ROW_SRCLR);

  // start selecting rows
  digitalWrite(ROW_SER, HIGH);

  for (int yCount = 0; yCount < ROW_COUNT; yCount++) {
    int y = ROW_COUNT - 1 - yCount;
    // brigthness - pushing data takes 40us, so to maximize brightness (at high brightness phases)
    // we want to keep the matrix on during update (except during latch). At low brightness phases,
    // we want it off to actually be dim
    bool brightPhase = brightnessPhase >= 2;
    // digitalWrite(ROW_OE, !brightPhase);

    // next row
    pulsePin(ROW_SRCLK);
    // only one row
    digitalWrite(ROW_SER, LOW);

    // we use 7/8 stages on shift registers + 1 is unused
    int moduleY = yCount % 20;
    if (moduleY == 0) {
      pulsePin(ROW_SRCLK);
    }

    if (moduleY == 7 || moduleY == 14 || (moduleY == 0 && yCount != 0)) {
      pulsePin(ROW_SRCLK);
    }

    // clear columns
    clearShiftReg(COL_SRCLK, COL_SRCLR);

    // set row data
    for (int x = 0; x < COL_COUNT; x++) {
      // get value
      // NOTE: values are loaded right-left
      uint8_t pxValue = framebuffer[y * ROW_COUNT + x];
      // apply brightness
      bool gotLight = (pxValue >> (4 + brightnessPhase)) & 1;
      // set value (note: inverted logic)
      gpio_put(COL_SER, !gotLight);
      // push value
      pulsePin(COL_SRCLK);

      // we use 7/8 stages on shift registers + 1 is unused
      int moduleX = x % 20;
      if (moduleX == 0) {
        pulsePin(COL_SRCLK);
      }
      if (moduleX == 6 || moduleX == 13 || moduleX == 19) {
        pulsePin(COL_SRCLK);
      }
    }

    // disable columns before latch
    outputEnable(ROW_OE, false);

    // latch rows and columns
    pulsePin(ROW_RCLK);
    pulsePin(COL_RCLK);

    // show for a certain period
    outputEnable(ROW_OE, true);
    busy_wait_us_32(brightnessPhaseDelays[brightnessPhase]);
    outputEnable(ROW_OE, false);
  }

  // next brightness phase
  brightnessPhase = (brightnessPhase + 1) % 4;
}
