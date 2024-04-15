#include <Arduino.h>
#include "gfx_png.h"
#include "lodepng.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "mbed_wait_api.h"

#define Serial Serial1

#define COL_SER 20
#define COL_OE 21
#define COL_RCLK 22
#define COL_SRCLK 26
#define COL_SRCLR 27

#define ROW_SER 14
#define ROW_OE 13
#define ROW_RCLK 12
#define ROW_SRCLK 11
#define ROW_SRCLR 10

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

#define ROW_COUNT 40
#define COL_COUNT 40

#define FPS 30
#define MS_PER_FRAME 1000 / FPS

uint16_t frameIndex = 0;
uint16_t lastRenderedFrameIndex = 0;
unsigned long frameLastChangedAt;

// we have 4-bit color depth, so 16 levels of brightness
// we go from phase 0 to phase 3
uint8_t brightnessPhase = 0;
uint8_t brightnessPhaseDelays[] = {1, 10, 30, 100};

uint8_t framebuffer[ROW_COUNT * COL_COUNT] = {0};

void main2();
void life_setup();
void life_step();
extern bool cells[ROW_COUNT * COL_COUNT];

void setup() {
  Serial.begin(115200);
  Serial.println("Hello worldd!");

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

  // clear frames
  frameIndex = 0;
  frameLastChangedAt = millis();

  // launch core1
  // NOTE: For some reason, without delay, core1 doesn't start?
  // delay(500);
  // multicore_reset_core1();
  // multicore_launch_core1(main2);

  // setup_audio();

  life_setup();

  // copy cells to framebuffer
  for (int y = 0; y < ROW_COUNT; y++) {
    for (int x = 0; x < COL_COUNT; x++) {
      framebuffer[y * ROW_COUNT + x] = cells[y * ROW_COUNT + x] ? 255 : 0;
    }
  }
}

void loop2();
void main2() {
  while (true) {
    loop2();
  }
}

void loop() {
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'p') {
      Serial.println("Paused. Press any key to continue.");
      outputEnable(ROW_OE, false);
      while (Serial.available() == 0) {
        Serial.read();
        delay(50);
      }
    } else if (c == 'r') {
      Serial.println("Restarting...");

    }
  }

  if (multicore_fifo_rvalid()) {
    uint32_t value = multicore_fifo_pop_blocking();
    if (value == 21372137) {
      Serial.println("Invalid frame size");
    } else {
      Serial.print("PNG decode error ");
      Serial.print(value);
      Serial.print(": ");
      Serial.println(lodepng_error_text(value));
    }
  }

  if (frameIndex != lastRenderedFrameIndex) {
    Serial.print("Going to frame ");
    Serial.println(frameIndex);
    lastRenderedFrameIndex = frameIndex;
  }

  // game of life step
  auto now = millis();
  if (now - frameLastChangedAt > 100) {
    frameLastChangedAt = now;
    life_step();
    for (int y = 0; y < ROW_COUNT; y++) {
      for (int x = 0; x < COL_COUNT; x++) {
        framebuffer[y * ROW_COUNT + x] = cells[y * ROW_COUNT + x] ? 255 : 0;
      }
    }
  }

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
    delayMicroseconds(brightnessPhaseDelays[brightnessPhase]);
    outputEnable(ROW_OE, false);
  }

  // next brightness phase
  brightnessPhase = (brightnessPhase + 1) % 4;
}

void loop2() {
  unsigned error;
  unsigned char *buffer = 0;
  unsigned width, height;

  // decode png
  const uint8_t *png = png_frames[frameIndex];
  size_t pngSize = png_frame_sizes[frameIndex];

  error = lodepng_decode_memory(&buffer, &width, &height, png, pngSize, LCT_GREY, 8);

  // push errors onto queue to be reported on core0, can't use serial here
  if (error) {
    free(buffer);
    multicore_fifo_push_blocking(error);
    return;
  } else if (width != ROW_COUNT || height != COL_COUNT) {
    free(buffer);
    multicore_fifo_push_blocking(21372137);
    return;
  }

  // copy to framebuffer
  // TODO: mutex? double buffer? or something...
  // TODO: learn to use memcpy lmao
  memcpy(framebuffer, buffer, ROW_COUNT * COL_COUNT);

  free(buffer);

  // wait until next frame
  // TODO: measure time to decode png
  busy_wait_ms(MS_PER_FRAME);

  frameIndex = (frameIndex + 1) % PNG_COUNT;
}
