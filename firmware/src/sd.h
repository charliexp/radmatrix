#pragma once

void setupSD();

bool isSDCardInserted();

extern size_t playlistSize;

void sd_loadPlaylist();
void sd_loadAudio(size_t index);
void sd_loadNextAudio();
bool sd_loadGfxFrameLengths(size_t index);
bool sd_loadGfxBlob(size_t index);
int32_t sd_loadNextFrame();
