#ifndef _text_h
#define _text_h

#include <stdint.h>
#include "config.h"

bool text_printChar(uint8_t framebuffer[ROW_COUNT * COL_COUNT], char c);
bool text_printStr(uint8_t framebuffer[ROW_COUNT * COL_COUNT], char *string);

#endif
