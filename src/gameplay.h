#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "shared.h"
#include "audio.h"

#include "../deps/raylib/src/raylib.h"

void gotoMainMenu(bool mainOrSelect);
bool mouseInRect(Rectangle rect);
void textBox(Rectangle rect, char *str, bool *selected);
void numberBox(Rectangle rect, int *number, bool *selected);
void slider(Rectangle rect, bool *selected, int *value, int max, int min);
bool drawInteractableButton(char *text, float fontScale, float x, float y, float width, float height);
void removeNote(int index);
int newNote(float time);
void removeSelectedNote(int index);
void addSelectNote(int note);
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
float getMusicHead();
bool isAnyKeyDown();
void fIntro();

#endif