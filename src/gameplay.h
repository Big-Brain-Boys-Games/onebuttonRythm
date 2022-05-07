#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "shared.h"
#include "audio.h"

#include "include/raylib.h"

void gotoMainMenu();
bool mouseInRect(Rectangle rect);
void removeNote(int index);
void newNote(float time);
void fPause();
void fCountDown();
void fMainMenu();
void fEndScreen();
void fEditor();
void fRecording();
void fPlaying();
void fFail();
void fNewMap();
void fMapSelect();

#endif