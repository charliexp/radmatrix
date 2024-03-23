#include <Arduino.h>
#include <FastLED.h>
#include "platform/mbed_wait_api.h"

// #define NUM_LEDS 1
// #define DATA_PIN 23u

// CRGB leds[NUM_LEDS];

#define Q1 5
#define Q2 29
#define Q3 28
#define Q4 27
#define Q5 26
#define Q6 22
#define Q7 21
#define Q8 20

#define Q9 8
#define Q10 19
#define Q11 18
#define Q12 17
#define Q13 16
#define Q14 9
#define Q15 10
#define Q16 11

#define Qout 12

#define SER 2
#define OE 3
#define RCLK 4
#define SRCLK 6
#define SRCLR 7



void printOut() {
    Serial.println("------");

    Serial.print("SER: ");
    Serial.println(digitalRead(SER));

    Serial.print("OE_N: ");
    Serial.println(digitalRead(OE));

    Serial.print("RCLK: ");
    Serial.println(digitalRead(RCLK));

    Serial.print("SRCLK: ");
    Serial.println(digitalRead(SRCLK));

    Serial.print("SRCLR_N: ");
    Serial.println(digitalRead(SRCLR));


  // uint16_t q = digitalRead(Q1) << 8 |
  //              digitalRead(Q2) << 7 |
  //              digitalRead(Q3) << 6 |
  //              digitalRead(Q4) << 5 |
  //              digitalRead(Q5) << 4 |
  //              digitalRead(Q6) << 3 |
  //              digitalRead(Q7) << 2 |
  //              digitalRead(Q8) << 1 |
  //              digitalRead(Qout);

  //   Serial.print("Q: ");
  //   Serial.println(q, BIN);
               Serial.println();

               Serial.print(digitalRead(Q1));
               Serial.print(digitalRead(Q2));
               Serial.print(digitalRead(Q3));
               Serial.print(digitalRead(Q4));
               Serial.print(digitalRead(Q5));
               Serial.print(digitalRead(Q6));
               Serial.print(digitalRead(Q7));
               Serial.print(digitalRead(Q8));
               Serial.print(" ");
               Serial.print(digitalRead(Q9));
                Serial.print(digitalRead(Q10));
                Serial.print(digitalRead(Q11));
                Serial.print(digitalRead(Q12));
                Serial.print(digitalRead(Q13));
                Serial.print(digitalRead(Q14));
                Serial.print(digitalRead(Q15));
                Serial.print(digitalRead(Q16));

               Serial.print(" ");
               Serial.print(digitalRead(Qout));
               Serial.println();
}

volatile uint64_t dupa;
volatile uint64_t dupa1;
volatile uint64_t dupa2;

void setup() {
  // pinMode(LED_BUILTIN, OUTPUT);
  // pinMode(24u, INPUT_PULLUP);
  // Serial.begin(9600);
  // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  // FastLED.setBrightness(30);

  dupa1 = 1231231232534;
  dupa2 = 23492839742;

  pinMode(0, OUTPUT);
  pinMode(1, OUTPUT);
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  digitalWrite(0, LOW);
  digitalWrite(1, LOW);
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);

  while(true) {
    digitalWrite(3, LOW);
    wait_ns(250);
    digitalWrite(0, HIGH);
    digitalWrite(1, HIGH);
    // digitalWrite(2, HIGH);
    // digitalWrite(4, HIGH);
    // digitalWrite(5, HIGH);

    // for(int i = 0; i < 300; i++) {
    //   dupa = dupa1 * dupa2;
    // }

    wait_ns(250);

    digitalWrite(3, HIGH);
    wait_ns(250);
    digitalWrite(0, LOW);
    digitalWrite(1, LOW);
    // digitalWrite(2, LOW);
    // digitalWrite(4, LOW);
    // digitalWrite(5, LOW);

    // for(int i = 0; i < 300; i++) {
    //   dupa = dupa1 * dupa2;
    // }

    wait_ns(250);
  }


  return

  pinMode(Q1, INPUT_PULLDOWN);
  pinMode(Q2, INPUT_PULLDOWN);
  pinMode(Q3, INPUT_PULLDOWN);
  pinMode(Q4, INPUT_PULLDOWN);
  pinMode(Q5, INPUT_PULLDOWN);
  pinMode(Q6, INPUT_PULLDOWN);
  pinMode(Q7, INPUT_PULLDOWN);
  pinMode(Q8, INPUT_PULLDOWN);
  pinMode(Q9, INPUT_PULLDOWN);
  pinMode(Q10, INPUT_PULLDOWN);
  pinMode(Q11, INPUT_PULLDOWN);
  pinMode(Q12, INPUT_PULLDOWN);
  pinMode(Q13, INPUT_PULLDOWN);
  pinMode(Q14, INPUT_PULLDOWN);
  pinMode(Q15, INPUT_PULLDOWN);
  pinMode(Q16, INPUT_PULLDOWN);
  pinMode(Qout, INPUT_PULLDOWN);

  // 1
  pinMode(SER, OUTPUT);
  // 2
  pinMode(OE, OUTPUT);
  digitalWrite(OE, HIGH);
  // 3
  pinMode(RCLK, OUTPUT);
  // 4
  pinMode(SRCLK, OUTPUT);
  // 5
  pinMode(SRCLR, OUTPUT);
  digitalWrite(SRCLR, HIGH);

  printOut();
}

// uint8_t r, g, b;

uint16_t prevQ = 255;

void loop() {
  // r += 3;
  // g += 5;
  // b += 7;

  // leds[0] = CRGB(r, g, b);
  // FastLED.show();
  // delay(100);

  // if (digitalRead(24u) == LOW) {
  //   Serial.println("Button pressed");
  // }

  char c = Serial.read();
  if (c == 'o') {
    printOut();
  }

  if (c == '1') {
    digitalWrite(SER, !digitalRead(SER));
    printOut();
  } else if (c == '2') {
    digitalWrite(OE, !digitalRead(OE));
    printOut();
  } else if (c == '3') {
    digitalWrite(RCLK, HIGH);
    digitalWrite(RCLK, LOW);
    printOut();
  } else if (c == '4') {
    digitalWrite(SRCLK, HIGH);
    digitalWrite(SRCLK, LOW);
    printOut();
  } else if (c == '5') {
    digitalWrite(SRCLR, !digitalRead(SRCLR));
    printOut();
  }
}
