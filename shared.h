#ifndef GAME_HELPER_FUNCTIONS
#define GAME_HELPER_FUNCTIONS

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <math.h>

// #include "include/miniaudio.h"

ma_decoder _decoder;
ma_device_config _deviceConfig;
ma_device _device;

// #include "include/raylib.h"
// #include "gameplay.h"

float noLessThanZero(float var);
void printAllnotes();
float getMusicDuration();
float getMusicPosition();
void fixMusicTime();
int getBarsCount();
int getBeatsCount();
float fDistance(float x1, float y1, float x2, float y2);
float musicTimeToScreen(float musicTime);
float screenToMusicTime(float x);
int getSamplePosition(float time);

#endif