#include <assert.h>
#include "text.h"

#include "font3x3.h"

#define FONT_WIDTH 3
#define FONT_HEIGHT 3

uint8_t cx = 0;
uint8_t cy = 0;

void text_setCursor(uint8_t x, uint8_t y) {
  assert(x < COL_COUNT);
  assert(y < ROW_COUNT);
  cx = x;
  cy = y;
}

uint8_t text_charIdx(char c) {
  // change to uppercase
  if (c >= 'a' && c <= 'z') {
    c += ('A' - 'a');
  }

  // find character
  for (int i = 0; i < sizeof(font); i++) {
    if (font[i].letter == c) {
      return i;
    }
  }

  // if not found, use `?` as placeholder
  return text_charIdx('?');
}

inline void text_newline() {
  cx = 0;
  cy += (FONT_HEIGHT + 1);
}

void text_next() {
  cx += (FONT_WIDTH + 1);
  if (cx >= COL_COUNT) {
    text_newline();
  }
}

bool text_printChar(uint8_t framebuffer[ROW_COUNT * COL_COUNT], char c) {
  // check if there is space
  if (cy + FONT_HEIGHT >= ROW_COUNT) {
    return false;
  }

  // special cases - space, newline
  if (c == '\n') {
    text_newline();
    return true;
  } else if (c == ' ') {
    text_next();
    return true;
  }

  // draw pixels
  uint8_t letterIdx = text_charIdx(c);
  for (int i = 0; i < FONT_HEIGHT; i++) {
    for (int j = 0; j < FONT_WIDTH; j++) {
      if (font[letterIdx].code[i][j] == '#') {
        framebuffer[(i + cy) * COL_COUNT + (j + cx)] = 255;
      }
    }
  }

  // move cursor
  text_next();
  return true;
}

bool text_printStr(uint8_t framebuffer[ROW_COUNT * COL_COUNT], char *string) {
  for (int i = 0; string[i] != 0; i++) {
    if (!text_printChar(framebuffer, string[i])) {
      return false;
    }
  }
  return true;
}
