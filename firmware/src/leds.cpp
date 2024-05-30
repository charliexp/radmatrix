#include <Arduino.h>
#include "hardware/gpio.h"
#include "mbed_wait_api.h"
#include "pico/multicore.h"
#include "hardware/pio.h"

#include "leds.h"
#include "leds.pio.h"

PIO leds_pio = pio0;
uint pusher_sm = 255; // invalid
uint delay_sm = 255; // invalid
uint row_sm = 255; // invalid

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
bool ledBufferReady = false;
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
  // pinMode(COL_RCLK, OUTPUT);
  pinMode(RCLK, OUTPUT);
  pinMode(COL_SRCLK, OUTPUT);
  pinMode(COL_SRCLR, OUTPUT);

  // set up row pins
  pinMode(ROW_SER, OUTPUT);
  pinMode(ROW_OE, OUTPUT);
  outputEnable(ROW_OE, false);
  // pinMode(ROW_RCLK, OUTPUT);
  pinMode(ROW_SRCLK, OUTPUT);
  pinMode(ROW_SRCLR, OUTPUT);

  // clear output - cols
  clearShiftReg(COL_SRCLK, COL_SRCLR);
  pulsePin(RCLK);
  outputEnable(COL_OE, true); // this is fine, because we control OE via rows only

  // clear output - rows
  clearShiftReg(ROW_SRCLK, ROW_SRCLR);
  pulsePin(RCLK);
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
  leds_initRowSelector();
  leds_initDelay();
  multicore_reset_core1();
  multicore_launch_core1(main2);
}

void leds_render() {
  if (!ledBufferReady) {
    outputEnable(ROW_OE, false);
    return;
  }

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

      // set row data using PIO
      // latch signal is also sent here
      // TODO: Some ideas for future optimization:
      // - see if we can disable px pusher delays on improved electric interface
      // - improve outer loop which adds 2us of processing on each loop
      // - change busy wait into some kind of interrupt-based thing so that processing can continue
      // - DMA?
      for (int xModule = 0; xModule < COL_MODULES; xModule++) {
        uint32_t pxValues = buffer[bufferOffset + xModule];
        pio_sm_put_blocking(leds_pio, pusher_sm, pxValues);
      }

      // wait until pushing and RCLK latch are done
      while (!pio_interrupt_get(leds_pio, 0)) {
        tight_loop_contents();
      }
      pio_interrupt_clear(leds_pio, pusher_sm);

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
  PIO pio = leds_pio;
  uint sm = pio_claim_unused_sm(pio, true);
  pusher_sm = sm;

  uint offset = pio_add_program(pio, &leds_px_pusher_program);

  pio_sm_config config = leds_px_pusher_program_get_default_config(offset);
  sm_config_set_clkdiv_int_frac(&config, 1, 0);

  // Shift OSR to the right, autopull
  sm_config_set_out_shift(&config, true, true, 32);

  // Set OUT (data) pin, connect to pad, set as output
  sm_config_set_out_pins(&config, COL_SER, 1);
  pio_gpio_init(pio, COL_SER);
  pio_sm_set_consecutive_pindirs(pio, sm, COL_SER, 1, true);

  // data is inverted
  gpio_set_outover(COL_SER, GPIO_OVERRIDE_INVERT);

  // Set sideset (SRCLK) pin, connect to pad, set as output
  sm_config_set_sideset_pins(&config, COL_SRCLK);
  pio_gpio_init(pio, COL_SRCLK);
  pio_sm_set_consecutive_pindirs(pio, sm, COL_SRCLK, 1, true);

  // Set SET (RCLK) pin, connect to pad, set as output
  sm_config_set_set_pins(&config, RCLK, 1);
  pio_gpio_init(pio, RCLK);
  pio_sm_set_consecutive_pindirs(pio, sm, RCLK, 1, true);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, sm, offset, &config);
  pio_sm_set_enabled(pio, sm, true);
}

void leds_initRowSelector() {
  PIO pio = leds_pio;
  uint sm = pio_claim_unused_sm(pio, true);
  row_sm = sm;

  uint offset = pio_add_program(pio, &leds_row_selector_program);

  pio_sm_config config = leds_row_selector_program_get_default_config(offset);
  sm_config_set_clkdiv_int_frac(&config, 1, 0);

  // Shift OSR to the right, autopull
  sm_config_set_out_shift(&config, true, true, 32);

  // Set OUT and SET (data) pin, connect to pad, set as output
  sm_config_set_out_pins(&config, ROW_SER, 1);
  sm_config_set_set_pins(&config, ROW_SER, 1);
  pio_gpio_init(pio, ROW_SER);
  pio_sm_set_consecutive_pindirs(pio, sm, ROW_SER, 1, true);

  // Set sideset (SRCLK) pin, connect to pad, set as output
  sm_config_set_sideset_pins(&config, ROW_SRCLK);
  pio_gpio_init(pio, ROW_SRCLK);
  pio_sm_set_consecutive_pindirs(pio, sm, ROW_SRCLK, 1, true);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, sm, offset, &config);
  pio_sm_set_enabled(pio, sm, true);
}

void leds_initDelay() {
  PIO pio = leds_pio;
  uint sm = pio_claim_unused_sm(pio, true);
  delay_sm = sm;

  uint offset = pio_add_program(pio, &leds_delay_program);

  pio_sm_config config = leds_delay_program_get_default_config(offset);
  sm_config_set_clkdiv_int_frac(&config, 1, 0);

  // Shift OSR to the right, autopull
  sm_config_set_out_shift(&config, true, true, 32);

  // Set sideset (OE) pin, connect to pad, set as output
  sm_config_set_sideset_pins(&config, ROW_OE);
  pio_gpio_init(pio, ROW_OE);
  pio_sm_set_consecutive_pindirs(pio, sm, ROW_OE, 1, true);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, sm, offset, &config);
  pio_sm_set_enabled(pio, sm, true);
}
