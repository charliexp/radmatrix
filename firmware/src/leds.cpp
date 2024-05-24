#include <Arduino.h>
#include "hardware/gpio.h"
#include "mbed_wait_api.h"
#include "pico/multicore.h"
#include "hardware/pio.h"

#include "leds.h"
#include "leds.pio.h"

PIO pusher_pio = pio0;
uint pusher_sm = 255; // invalid

// NOTE: RCLK, SRCLK capture on *rising* edge
inline void pulsePin(uint8_t pin) {
  gpio_put(pin, HIGH);
   // there are glitches without this (maybe just due to breadboard...)
  _NOP();
  _NOP();
  _NOP();
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
  outputEnable(ROW_OE, false);
  pinMode(COL_RCLK, OUTPUT);
  pinMode(COL_SRCLK, OUTPUT);
  pinMode(COL_SRCLR, OUTPUT);

  // set up row pins
  pinMode(ROW_SER, OUTPUT);
  pinMode(ROW_OE, OUTPUT);
  outputEnable(ROW_OE, false);
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

void leds_initPusher();

void leds_initRenderer() {
  leds_initPusher();
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
    digitalWrite(ROW_OE, !brightPhase);

    // next row
    gpio_put(ROW_SRCLK, HIGH);
    busy_wait_us_32(1);
    gpio_put(ROW_SRCLK, LOW);
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

    // set row data
    size_t rowOffset = y * COL_COUNT;
    for (int x = 0; x < COL_COUNT; x++) {
      // get value
      // NOTE: values are loaded right-left
      uint8_t pxValue = framebuffer[rowOffset + x];

      // we use 7/8 stages on shift registers + 1 is unused
      int moduleX = x % 20;
      if (moduleX == 0) {
        pio_sm_put_blocking(pusher_pio, pusher_sm, 0);
      }
      if (moduleX == 6 || moduleX == 13 || moduleX == 19) {
        pio_sm_put_blocking(pusher_pio, pusher_sm, 0);
      }

      // apply brightness
      bool gotLight = (pxValue >> (4 + brightnessPhase)) & 1;
      // push value
      pio_sm_put_blocking(pusher_pio, pusher_sm, gotLight);
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

void leds_initPusher() {
  PIO pio = pusher_pio;
  uint sm = pio_claim_unused_sm(pio, true);
  pusher_sm = sm;

  uint offset = pio_add_program(pio, &leds_px_pusher_program);

  uint dataPin = COL_SER;
  uint latchPin = COL_SRCLK;

  pio_sm_config config = leds_px_pusher_program_get_default_config(offset);
  sm_config_set_clkdiv_int_frac(&config, 1, 0);

  // Set OUT (data) pin, connect to pad, set as output
  sm_config_set_out_pins(&config, dataPin, 1);
  pio_gpio_init(pio, dataPin);
  pio_sm_set_consecutive_pindirs(pio, sm, dataPin, 1, true);

  // data is inverted
  gpio_set_outover(dataPin, GPIO_OVERRIDE_INVERT);

  // Set SET (latch) pin, connect to pad, set as output
  sm_config_set_sideset_pins(&config, latchPin);
  pio_gpio_init(pio, latchPin);
  pio_sm_set_consecutive_pindirs(pio, sm, latchPin, 1, true);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, sm, offset, &config);
  pio_sm_set_enabled(pio, sm, true);
}
