#pragma once

#ifndef _can_h
#define _can_h

extern "C" {
  #include "can2040.h"
}

static struct can2040 canbus;

typedef struct {
  bool wants_next_song;
} canbus_status;

void canbus_setup();
canbus_status canbus_loop();

#endif
