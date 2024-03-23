#include <Arduino.h>
#include "gfx.h"

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

uint8_t frameIndex = 0;
uint16_t frameLastChangedAt;

#define MS_PER_FRAME 150

void printOut() {
    Serial.println("------");
    Serial.println("Row:");

    Serial.print("SER: ");
    Serial.println(digitalRead(ROW_SER));

    Serial.print("OE_N: ");
    Serial.println(digitalRead(ROW_OE));

    Serial.print("RCLK: ");
    Serial.println(digitalRead(ROW_RCLK));

    Serial.print("SRCLK: ");
    Serial.println(digitalRead(ROW_SRCLK));

    Serial.print("SRCLR_N: ");
    Serial.println(digitalRead(ROW_SRCLR));

    Serial.println("------");
    Serial.println("Col:");

    Serial.print("SER: ");
    Serial.println(digitalRead(COL_SER));

    Serial.print("OE_N: ");
    Serial.println(digitalRead(COL_OE));

    Serial.print("RCLK: ");
    Serial.println(digitalRead(COL_RCLK));

    Serial.print("SRCLK: ");
    Serial.println(digitalRead(COL_SRCLK));

    Serial.print("SRCLR_N: ");
    Serial.println(digitalRead(COL_SRCLR));
}

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
}

void loop() {
  char c = Serial.read();
  if (c == 'o') {
    printOut();
  }

  uint32_t *frame = frames[frameIndex];

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
      bool pxValue = frame[ROW_COUNT - 1 - y] & (1 << ((COL_COUNT - 1) - x));
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

  // next frame
  if (millis() - frameLastChangedAt > MS_PER_FRAME) {
    frameLastChangedAt = millis();
    frameIndex++;
    if (frameIndex == GFX_COUNT) {
      frameIndex = 0;
    }
  }
}
