#ifndef GAME_AUDIO_INTERFACE
#define GAME_AUDIO_INTERFACE

void loadMusic(char * file);

float getMusicDuration();
float getMusicPosition();
void fixMusicTime();
int getBarsCount();
int getBeatsCount();
void setMusicFrameCount();

void audioInit();


int getSamplePosition(float time);

#endif