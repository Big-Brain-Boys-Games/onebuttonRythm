#ifndef GAME_AUDIO_INTERFACE
#define GAME_AUDIO_INTERFACE

#include <stdbool.h>
#include "files.h"

int getSamplePosition(float time);
float getMusicDuration();
float getMusicPosition();
void fixMusicTime();
int getBarsCount();
int getBeatsCount();
void setMusicStart();
void randomMusicPoint();
void audioInit();
void loadMusic(Map * map);
void loadAudio(void ** buffer, char * file, int * audioLength);
bool endOfMusic();
void playAudioEffect(void *effect, int size);
void startMusic();
void stopMusic();
void setMusicFrameCount();


#endif