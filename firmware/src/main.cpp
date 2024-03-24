#include <Arduino.h>
#include "gfx_png.h"
#include "lodepng.h"
#include "pico/multicore.h"
#include "pico/runtime.h"
#include "mbed_wait_api.h"
#include <malloc.h>

#define COL_SER 0
#define COL_OE 26
#define COL_RCLK 27
#define COL_SRCLK 28
#define COL_SRCLR 29

#define ROW_SER 8
#define ROW_OE 7
#define ROW_RCLK 6
#define ROW_SRCLK 5
#define ROW_SRCLR 4

#define ROW_COUNT 20 //24
#define COL_COUNT 20 //21

#define MS_PER_FRAME 33

uint16_t frameIndex = 0;
uint16_t lastRenderedFrameIndex = 0;

// we have 4-bit color depth, so 16 levels of brightness
// we go from phase 0 to phase 3
uint8_t brightnessPhase = 0;
uint8_t brightnessPhaseDelays[] = {10, 30, 80, 200};

uint8_t framebuffer[ROW_COUNT * COL_COUNT]; // = {0};

uint32_t getTotalHeap(void) {
  extern char __StackLimit, __bss_end__;
  return &__StackLimit  - &__bss_end__;
}

uint32_t getFreeHeap(void) {
  struct mallinfo m = mallinfo();
  return getTotalHeap() - m.uordblks;
}

void main2();
void setup() {
  Serial.begin(9600);
  Serial.println("Hello");

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
  digitalWrite(COL_SER, LOW);
  digitalWrite(COL_OE, HIGH);

  digitalWrite(COL_SRCLR, LOW);
  digitalWrite(COL_RCLK, HIGH);
  digitalWrite(COL_RCLK, LOW);
  digitalWrite(COL_SRCLR, HIGH);

  digitalWrite(COL_OE, LOW);

  // clear output - rows
  digitalWrite(ROW_SER, LOW);
  digitalWrite(ROW_OE, HIGH);

  digitalWrite(ROW_SRCLR, LOW);
  digitalWrite(ROW_RCLK, HIGH);
  digitalWrite(ROW_RCLK, LOW);
  digitalWrite(ROW_SRCLR, HIGH);

  digitalWrite(ROW_OE, LOW);

  // clear frames
  frameIndex = 0;
  // frameLastChangedAt = millis();

  // launch core1
  // NOTE: For some reason, without delay, core1 doesn't start?
  delay(500);
  multicore_reset_core1();
  multicore_launch_core1(main2);
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
      digitalWrite(ROW_OE, HIGH);
      while (Serial.available() == 0) {
        Serial.read();
        delay(50);
      }
    } else if (c == 'r') {
      Serial.println("Restarting...");
      // TODO
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

  // clear columns
  digitalWrite(COL_SRCLR, LOW);
  digitalWrite(COL_SRCLK, HIGH);
  digitalWrite(COL_SRCLK, LOW);
  digitalWrite(COL_SRCLR, HIGH);

  // start selecting columns
  digitalWrite(COL_SER, HIGH);

  // temp: hide
  digitalWrite(ROW_OE, HIGH);

  for (int x = 0; x < COL_COUNT; x++) {
    // next column
    digitalWrite(COL_SRCLK, HIGH);
    digitalWrite(COL_SRCLK, LOW);
    // only one column
    digitalWrite(COL_SER, LOW);
    // we use 7/8 stages on shift registers for columns
    if (x % 7 == 0) {
      digitalWrite(COL_SRCLK, HIGH);
      digitalWrite(COL_SRCLK, LOW);
    }

    // clear row
    digitalWrite(ROW_SRCLR, LOW);
    digitalWrite(ROW_SRCLK, HIGH);
    digitalWrite(ROW_SRCLK, LOW);
    digitalWrite(ROW_SRCLR, HIGH);

    // set column with rows' data
    for (int y = 0; y < ROW_COUNT; y++) {
      // get value
      uint8_t pxValue = framebuffer[y * COL_COUNT + x];
      // apply brightness
      bool gotLight = (pxValue >> (4 + brightnessPhase)) & 1;
      digitalWrite(ROW_SER, gotLight);
      // push value
      digitalWrite(ROW_SRCLK, HIGH);
      digitalWrite(ROW_SRCLK, LOW);
    }
    // disable rows before latch
    // digitalWrite(ROW_OE, HIGH);
    // latch column
    digitalWrite(COL_RCLK, HIGH);
    digitalWrite(COL_RCLK, LOW);
    // latch rows
    digitalWrite(ROW_RCLK, HIGH);
    digitalWrite(ROW_RCLK, LOW);
    // enable rows after latch
    // digitalWrite(ROW_OE, LOW);

    // note: 40us per column

    // show for a certain period
    digitalWrite(ROW_OE, LOW);
    delayMicroseconds(brightnessPhaseDelays[brightnessPhase]);
    digitalWrite(ROW_OE, HIGH);
  }

  // next brightness phase
  brightnessPhase = (brightnessPhase + 1) % 4;
}

// second core - png decoding
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
  } else if (width != COL_COUNT || height != ROW_COUNT) {
    free(buffer);
    multicore_fifo_push_blocking(21372137);
    return;
  }

  // copy to framebuffer
  // TODO: mutex? double buffer? or something...
  // TODO: learn to use memcpy lmao
  for (int y = ROW_COUNT - 1; y >= 0; y--) {
    for (int x = 0; x < COL_COUNT; x++) {
      framebuffer[y * COL_COUNT + x] = buffer[(ROW_COUNT - 1 - y) * COL_COUNT + x];
    }
  }

  free(buffer);

  // wait until next frame
  // TODO: measure time to decode png
  busy_wait_ms(MS_PER_FRAME);

  frameIndex = (frameIndex + 1) % PNG_COUNT;
}
