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

// we have COLOR_BITS-bit color depth, so 2^COLOR_BITS levels of brightness
// we go from phase 0 to phase (COLOR_BITS-1)
uint8_t brightnessPhase = 0;
uint8_t brightnessPhaseDelays[COLOR_BITS] = {0, 1, 6, 20, 60};

// NOTE: Alignment required to allow 4-byte reads
uint8_t framebuffer[ROW_COUNT * COL_COUNT]  __attribute__((aligned(32))) = {0};
uint32_t ledBuffer[8][ROW_COUNT * COL_MODULES] = {0};

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
  // brightness phase
  bool brightPhase = brightnessPhase >= 3;
  auto buffer = ledBuffer[brightnessPhase + 3];

  // hide output
  outputEnable(ROW_OE, false);

  // clear rows
  clearShiftReg(ROW_SRCLK, ROW_SRCLR);

  // start selecting rows
  gpio_put(ROW_SER, HIGH);

  int bufferOffset = 0;
  for (int yModule = 0; yModule < ROW_MODULES; yModule++) {
    for (int moduleY = 0; moduleY < 20; moduleY++) {
      // brigthness - pushing data takes time, so to maximize brightness (at high brightness phases)
      // we want to keep the matrix on during update (except during latch). At low brightness phases,
      // we want it off to actually be dim
      outputEnable(ROW_OE, brightPhase);

      // next row
      pulsePin(ROW_SRCLK);
      // only one row
      gpio_put(ROW_SER, LOW);

      // we use 7/8 stages on shift registers + 1 is unused
      if (moduleY == 0) {
        pulsePin(ROW_SRCLK);
      }

      if (moduleY == 7 || moduleY == 14 || (moduleY == 0 && yModule != 0)) {
        pulsePin(ROW_SRCLK);
      }

      // set row data
      // NOTE: values are loaded right-left
      // Optimized implementation: use PIO, avoid division, modulo, etc...
      // we use 7/8 stages of each shift register + 1 is unused so we need to do
      // silly shit
      // TODO: Some ideas for future optimization:
      // - see if we can disable px pusher delays on improved electric interface
      // - improve outer loop which adds 2us of processing on each loop
      // - change busy wait into some kind of interrupt-based thing so that processing can continue
      // - latch row and clock simultaneously, avoid disabling output
      // - DMA?
      for (int xModule = 0; xModule < COL_MODULES; xModule++) {
        uint32_t pxValues = buffer[bufferOffset + xModule];
        pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues);
      }

      // wait for all data to be shifted out
      while (!pio_sm_is_tx_fifo_empty(pusher_pio, pusher_sm)) {
        tight_loop_contents();
      }
      // TODO: Is there an API to wait for PIO to actually become idle?
      // pio_sm_drain_tx_fifo doesn't seem to do the trick
      // if not, we might need to use irqs or something
      busy_wait_us(4);

      // latch rows and columns
      gpio_set_mask(1 << ROW_RCLK | 1 << COL_RCLK);
      _NOP();
      _NOP();
      gpio_clr_mask(1 << ROW_RCLK | 1 << COL_RCLK);

      // show for a certain period
      outputEnable(ROW_OE, true);
      busy_wait_us_32(brightnessPhaseDelays[brightnessPhase]);
      outputEnable(ROW_OE, false);

      // next row
      bufferOffset += COL_MODULES;
    }
  }

  // next brightness phase
  brightnessPhase = (brightnessPhase + 1) % COLOR_BITS;
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

  // Shift OSR to the right, autopull
  sm_config_set_out_shift(&config, true, true, 32);

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
