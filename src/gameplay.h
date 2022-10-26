#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "files.h"
#include "audio.h"

#include "../deps/raylib/src/raylib.h"


TimingSegment getTimingSignature(float time);
TimingSegment * addTimingSignature(float time, int bpm);
void removeTimingSignature(float time);
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
void fPause(bool reset);
void fCountDown(bool reset);
void fMainMenu(bool reset);
void fSettings(bool reset);
void fEndScreen(bool reset);
void fEditor(bool reset);
void fRecording(bool reset);
void fPlaying(bool reset);
void fFail(bool reset);
void fNewMap(bool reset);
void fMapSelect(bool reset);
void fExport(bool reset);
float getMusicHead();
bool isAnyKeyDown();
void fIntro(bool reset);

#endif