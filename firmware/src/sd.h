#pragma once

void setupSDPins();
void setupSD();

bool isSDCardInserted();

void sd_getAudio();
bool sd_loadGfxFrameLengths();
bool sd_loadGfxBlob();
int32_t sd_loadNextFrame();
