#include <Arduino.h>
#include "gfx_png.h"
#include "lodepng.h"
#include "pico/multicore.h"
#include "mbed_wait_api.h"

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
unsigned long frameLastChangedAt;

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
  frameLastChangedAt = millis();

  // launch core1
  // NOTE: For some reason, without delay, core1 doesn't start?
  delay(300);
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
  // Serial.println(multicore_fifo_get_status());
  if (multicore_fifo_rvalid()) {
    uint32_t value = multicore_fifo_pop_blocking();
    Serial.print("Core 1 says: ");
    Serial.println(value);
  }

  unsigned error;
  unsigned char *buffer = 0;
  unsigned width, height;

  const uint8_t *png = png_frames[frameIndex];
  size_t pngSize = png_frame_sizes[frameIndex];

  error = lodepng_decode_memory(&buffer, &width, &height, png, pngSize, LCT_GREY, 8);

  if (error) {
    Serial.print("PNG decode error ");
    Serial.print(error);
    Serial.print(": ");
    Serial.println(lodepng_error_text(error));
    return;
  } else if (width != COL_COUNT || height != ROW_COUNT) {
    Serial.println("Invalid frame size");
    return;
  }

  // clear columns
  digitalWrite(COL_SRCLR, LOW);
  digitalWrite(COL_SRCLK, HIGH);
  digitalWrite(COL_SRCLK, LOW);
  digitalWrite(COL_SRCLR, HIGH);

  // start selecting columns
  digitalWrite(COL_SER, HIGH);

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
      uint8_t pxValue = buffer[y * COL_COUNT + x] > 128 ? 1 : 0;
      digitalWrite(ROW_SER, pxValue);
      // push value
      digitalWrite(ROW_SRCLK, HIGH);
      digitalWrite(ROW_SRCLK, LOW);
    }
    // disable rows before latch
    digitalWrite(ROW_OE, HIGH);
    // latch column
    digitalWrite(COL_RCLK, HIGH);
    digitalWrite(COL_RCLK, LOW);
    // latch rows
    digitalWrite(ROW_RCLK, HIGH);
    digitalWrite(ROW_RCLK, LOW);
    // enable rows after latch
    digitalWrite(ROW_OE, LOW);
  }

  free(buffer);

  // next frame
  if (millis() - frameLastChangedAt > MS_PER_FRAME) {
    frameLastChangedAt = millis();
    frameIndex++;
    Serial.print("Going to frame ");
    Serial.println(frameIndex);
    if (frameIndex == PNG_COUNT) {
      frameIndex = 0;
      Serial.println("Loop over!");
    }
  }
}

uint32_t counter = 0;

void loop2() {
  counter += 1;
  if (multicore_fifo_wready()) {
    multicore_fifo_push_blocking(counter);
  }
  busy_wait_us(500000);
}
