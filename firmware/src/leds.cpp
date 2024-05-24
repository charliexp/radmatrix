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

// we have 4-bit color depth, so 16 levels of brightness
// we go from phase 0 to phase 3
uint8_t brightnessPhase = 0;
uint8_t brightnessPhaseDelays[] = {1, 10, 30, 100};

// NOTE: Alignment required to allow 4-byte reads
uint8_t framebuffer[ROW_COUNT * COL_COUNT]  __attribute__((aligned(32))) = {0};

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
  gpio_put(ROW_SER, HIGH);

  for (int yCount = 0; yCount < ROW_COUNT; yCount++) {
    int y = ROW_COUNT - 1 - yCount;
    // brigthness - pushing data takes 40us, so to maximize brightness (at high brightness phases)
    // we want to keep the matrix on during update (except during latch). At low brightness phases,
    // we want it off to actually be dim
    bool brightPhase = brightnessPhase >= 2;
    outputEnable(ROW_OE, brightPhase);

    // next row
    pulsePin(ROW_SRCLK);
    // only one row
    gpio_put(ROW_SER, LOW);

    // we use 7/8 stages on shift registers + 1 is unused
    int moduleY = yCount % 20;
    if (moduleY == 0) {
      pulsePin(ROW_SRCLK);
    }

    if (moduleY == 7 || moduleY == 14 || (moduleY == 0 && yCount != 0)) {
      pulsePin(ROW_SRCLK);
    }

    // set row data
    // NOTE: values are loaded right-left
    // Optimized implementation: use PIO, avoid division, modulo, etc...
    // we use 7/8 stages of each shift register + 1 is unused so we need to do
    // silly shit
    // TODO: Some ideas for future optimization:
    // - see if we can disable px pusher delays on improved electric interface
    // - use a profiler to see how the inner loop can be improved
    // - do the shift register bullshit once per frame, so that data can be loaded into
    //   registers with aligned access, DMA, etc.
    // - improve outer loop which adds 2us of processing on each loop
    // - change busy wait into some kind of interrupt-based thing so that processing can continue
    // - latch row and clock simultaneously, avoid disabling output
    uint8_t *buffer = framebuffer + (y * COL_COUNT);
    for (int xModule = 0; xModule < COL_MODULES; xModule++) {
      uint32_t pxValues;

      // placeholder at 0; pixels 0, 1, 2
      pxValues = *(reinterpret_cast<uint32_t *>(buffer));
      pxValues = pxValues << 8;
      pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues >> brightnessPhase);

      // pixels 3, 4, 5, placeholder at 6
      pxValues = *(reinterpret_cast<uint32_t *>(buffer + 3));
      pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues >> brightnessPhase);

      // pixels 6, 7, 8, 9
      pxValues = *(reinterpret_cast<uint32_t *>(buffer + 6));
      pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues >> brightnessPhase);

      // pixels 10, 11, 12, placeholder at 13
      pxValues = *(reinterpret_cast<uint32_t *>(buffer + 10));
      pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues >> brightnessPhase);

      // pixels 13, 14, 15, 16
      pxValues = *(reinterpret_cast<uint32_t *>(buffer + 13));
      pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues >> brightnessPhase);

      // pixels 17, 18, 19, placeholder
      pxValues = *(reinterpret_cast<uint32_t *>(buffer + 17));
      pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues >> brightnessPhase);

      buffer += 20;
    }

    // wait for all data to be shifted out
    pio_sm_drain_tx_fifo(pusher_pio, pusher_sm);

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
