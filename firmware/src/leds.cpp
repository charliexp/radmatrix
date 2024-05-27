#include <Arduino.h>
#include "hardware/gpio.h"
#include "mbed_wait_api.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/irq.h"

#include "leds.h"
#include "leds.pio.h"

PIO pusher_pio = pio0;
uint pusher_sm = 255; // invalid

#define PWM_SLICE 0
volatile bool delayFinished = true;

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
// in nanoseconds
uint16_t brightnessPhaseDelays[COLOR_BITS] = {500, 1500, 3000, 20000, 60000};

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

void leds_initPusher();
void leds_init_pwm();

void main2() {
  leds_initPusher();
  leds_init_pwm();
  while (true) {
    leds_render();
  }
}

void leds_initRenderer() {
  multicore_reset_core1();
  multicore_launch_core1(main2);
}

void leds_start_delay();

void leds_render() {
  if (!ledBufferReady) {
    outputEnable(ROW_OE, false);
    return;
  }

  // brightness phase
  auto buffer = ledBuffer[brightnessPhase + 3];

  // configure delays
  pwm_set_clkdiv_int_frac(PWM_SLICE, 1, 0);
  // 8ns per cycle at 125MHz
  auto delayTicks = brightnessPhaseDelays[brightnessPhase] / 8;
  pwm_set_wrap(PWM_SLICE, delayTicks);

  // hide output
  outputEnable(ROW_OE, false);

  // clear rows
  clearShiftReg(ROW_SRCLK, ROW_SRCLR);

  // start selecting rows
  gpio_put(ROW_SER, HIGH);

  int bufferOffset = 0;
  for (int yModule = 0; yModule < ROW_MODULES; yModule++) {
    for (int moduleY = 0; moduleY < 20; moduleY++) {
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
        pio_sm_put_blocking(pusher_pio, pusher_sm, pxValues);
      }

      // wait until previous row's delay is done
      while (!delayFinished) {
        tight_loop_contents();
      }

      // allow pusher to latch data
      pusher_pio->irq_force = 1 << 0;

      // wait until pushing and RCLK latch are done
      while (!pio_interrupt_get(pusher_pio, 1)) {
        tight_loop_contents();
      }
      pio_interrupt_clear(pusher_pio, 1);

      // enable output for specified time
      leds_start_delay();

      // next row
      bufferOffset += COL_MODULES;
    }
  }

  // wait until last row's delay is done
  while (!delayFinished) {
    tight_loop_contents();
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

  // Set sideset (SRCLK) pin, connect to pad, set as output
  sm_config_set_sideset_pins(&config, latchPin);
  pio_gpio_init(pio, latchPin);
  pio_sm_set_consecutive_pindirs(pio, sm, latchPin, 1, true);

  // Set SET (RCLK) pin, connect to pad, set as output
  sm_config_set_set_pins(&config, RCLK, 1);
  pio_gpio_init(pio, RCLK);
  pio_sm_set_consecutive_pindirs(pio, sm, RCLK, 1, true);

  // Load our configuration, and jump to the start of the program
  pio_sm_init(pio, sm, offset, &config);
  pio_sm_set_enabled(pio, sm, true);
}

void leds_start_delay() {
  // enable output, start PWM counter
  delayFinished = false;
  outputEnable(ROW_OE, true);
  pwm_set_enabled(PWM_SLICE, true);
}

void leds_pwm_interrupt_handler() {
  // disable output
  outputEnable(ROW_OE, false);
  // stop PWM counter
  pwm_set_enabled(PWM_SLICE, false);
  // acknowledge interrupt
  pwm_clear_irq(PWM_SLICE);
  delayFinished = true;
}

// We will use PWM to control OE signal with a precise delay
// Note that we only use PWM for timing, it doesn't actually drive the OE GPIO
// (timers are microsecond-resolution, and we need better than this)
void leds_init_pwm() {
  irq_set_exclusive_handler(PWM_IRQ_WRAP, leds_pwm_interrupt_handler);
  irq_set_enabled(PWM_IRQ_WRAP, true);

  pwm_clear_irq(PWM_SLICE);
  pwm_set_irq_enabled(PWM_SLICE, true);
}
