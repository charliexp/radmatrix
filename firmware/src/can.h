#pragma once

#ifndef _can_h
#define _can_h

extern "C" {
  #include "can2040.h"
}

static struct can2040 canbus;

void canbus_setup();
void canbus_loop();

#endif
