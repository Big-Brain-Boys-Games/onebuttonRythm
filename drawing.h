#ifndef GAME_DRAW_OBJECTS
#define GAME_DRAW_OBJECTS

#ifndef MAINC
#include "shared.h"
#include "files.h"
#include "gameplay.h"
#endif

#include "include/raylib.h"

float musicTimeToScreen(float musicTime);
float screenToMusicTime(float x);
float noteFadeOut(float note);
void drawCursor();
void dNotes();
void drawMapThumbnail (Rectangle rect, Map *map);
void drawBars();
void drawBackground();
void drawVignette();
void drawProgressBar();
void drawProgressBarI(bool interactable);
void drawButton(Rectangle rect, char * text, float fontScale);
void fMapSelect();
void drawMusicGraph(float transparent);

#endif