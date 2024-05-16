#include <Arduino.h>
#include "gfx_png.h"
#include "lodepng.h"
#include "pico/multicore.h"
#include "hardware/gpio.h"
#include "mbed_wait_api.h"
#include <SPI.h>
// #include <SD.h>
#include <RP2040_SD.h>
// #include "audio.h"

// #define Serial Serial1

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

void printDirectory(File dir, int numTabs);

void setup() {
  pinMode(16, INPUT);
  pinMode(17, OUTPUT);
  digitalWrite(17, HIGH);
  pinMode(18, OUTPUT);
  pinMode(19, OUTPUT);
  pinMode(28, INPUT_PULLUP);
  // pinMode(23, OUTPUT);

  delay(2000);
  Serial.begin(115200);
  Serial.println("Hello worldd!");


  SPI.begin();

  while (digitalRead(28) == HIGH) {
    Serial.println("SD card not connected, waiting...");
    delay(1000);
  }

  Serial.println(BOARD_NAME);
   Serial.println(RP2040_SD_VERSION);

   Serial.print("Initializing SD card with SS = ");
   Serial.println(PIN_SPI_SS);
   Serial.print("SCK = ");
   Serial.println(PIN_SPI_SCK);
   Serial.print("MOSI = ");
   Serial.println(PIN_SPI_MOSI);
   Serial.print("MISO = ");
   Serial.println(PIN_SPI_MISO);

   if (!SD.begin(PIN_SPI_SS))
   {
     Serial.println("Initialization failed!");
    Serial.print("Error code: ");
    // Serial.println(SD.card.errorCode(), HEX);
     return;
   }

  Serial.println("initialization done.");

  // delay(2000);
  // return;

   // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File myFile = SD.open("test2.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("testing 1, 2, 3.");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("test2.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
/*
  Serial.print("Clusters:          ");
  Serial.println(SD.volume.clusterCount());
  Serial.print("Blocks x Cluster:  ");
  Serial.println(SD.volume.blocksPerCluster());

  Serial.print("Total Blocks:      ");
  Serial.println(SD.volume.blocksPerCluster() * SD.volume.clusterCount());
  Serial.println();

  // print the type and size of the first FAT-type volume
  uint32_t volumesize;
  Serial.print("Volume type is:    FAT");
  Serial.println(SD.volume.fatType(), DEC);

  volumesize = SD.volume.blocksPerCluster();    // clusters are collections of blocks
  volumesize *= SD.volume.clusterCount();       // we'll have a lot of clusters
  volumesize /= 2;                           // SD card blocks are always 512 bytes (2 blocks are 1 KB)
  Serial.print("Volume size (KB):  ");
  Serial.println(volumesize);
  Serial.print("Volume size (MB):  ");
  volumesize /= 1024;
  Serial.println(volumesize);
  Serial.print("Volume size (GB):  ");
  Serial.println((float)volumesize / 1024.0);*/

  File root = SD.open("/");

  printDirectory(root, 0);

  // Serial.println("\nFiles found on the card (name, date and size in bytes): ");
  // SD.root.openRoot(SD.volume);

  // list all files in the card with date and size
  // SD.root.ls(LS_R | LS_DATE | LS_SIZE);
  // SD.root.close();

/*
  Serial.print("Initializing SD card...");

  if (!SD.begin(17)) {
    Serial.println("initialization failed!");
    Serial.print("Error code: ");
    Serial.println(SD.card.errorCode(), HEX);
    while (1);
  }
*/

  // if (!SD.begin(17)) {
  //   Serial.println("initialization failed!");
  //   Serial.print("Error code: ");
  //   Serial.println(SD.card.errorCode(), HEX);
  //   while (1);
  // }

  return;

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
  delay(500);
  multicore_reset_core1();
  multicore_launch_core1(main2);

  // setup_audio();

  // life_setup();

  // // copy cells to framebuffer
  // for (int y = 0; y < ROW_COUNT; y++) {
  //   for (int x = 0; x < COL_COUNT; x++) {
  //     framebuffer[y * ROW_COUNT + x] = cells[y * ROW_COUNT + x] ? 255 : 0;
  //   }
  // }
}

void printDirectory(File dir, int numTabs) {

  while (true) {

    File entry =  dir.openNextFile();

    if (! entry) {

      // no more files

      break;

    }

    for (uint8_t i = 0; i < numTabs; i++) {

      Serial.print('\t');

    }

    Serial.print(entry.name());

    if (entry.isDirectory()) {

      Serial.println("/");

      printDirectory(entry, numTabs + 1);

    } else {

      // files have sizes, directories do not

      Serial.print("\t\t");

      Serial.println(entry.size(), DEC);

    }

    entry.close();

  }
}

void loop2();
void main2() {
  while (true) {
    loop2();
  }
}

void loop() {

  return;
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
  // auto now = millis();
  // if (now - frameLastChangedAt > 100) {
  //   frameLastChangedAt = now;
  //   life_step();
  //   for (int y = 0; y < ROW_COUNT; y++) {
  //     for (int x = 0; x < COL_COUNT; x++) {
  //       framebuffer[y * ROW_COUNT + x] = cells[y * ROW_COUNT + x] ? 255 : 0;
  //     }
  //   }
  // }

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
