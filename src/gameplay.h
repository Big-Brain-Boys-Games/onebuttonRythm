#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "shared.h"
#include "audio.h"

#include "include/raylib.h"

void gotoMainMenu();
bool mouseInRect(Rectangle rect);
void textBox(Rectangle rect, char * str, bool * selected);
void numberBox(Rectangle rect, int * number, bool * selected);
Rectangle drawInteractableButton(char * text, float fontScale, float x, float y,float width,float height);
void removeNote(int index);
void newNote(float time);
void fPause();
void fCountDown();
void fMainMenu();
void fSettings();
void fEndScreen();
void fEditor();
void fRecording();
void fPlaying();
void fFail();
void fNewMap();
void fMapSelect();
void fExport();

#endif