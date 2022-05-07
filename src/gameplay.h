#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "shared.h"
#include "audio.h"

#include "include/raylib.h"

bool endOfMusic();
void resetBackGround();
void playAudioEffect();
void startMusic();
void stopMusic();
bool mouseInRect(Rectangle rect);
void removeNote(int index);
void newNote(float time);
void fMainMenu();
void fEditor();
void fCountDown();
void fRecording();
void fPlaying();
void fFail();
void fNewMap();

#endif