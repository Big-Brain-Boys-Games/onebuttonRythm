#ifndef GAME_DRAW_OBJECTS
#define GAME_DRAW_OBJECTS

#include "shared.h"
#include "files.h"
#include "gameplay.h"

#include "deps/raylib/src/raylib.h"

int measureText (char * text, int fontSize);
void drawText(char * str, int x, int y, int fontSize, Color color);
float musicTimeToScreen(float musicTime);
float screenToMusicTime(float x);
float noteFadeOut(float note);
void drawCursor();
void dNotes();
void drawMapThumbnail(Rectangle rect, Map *map, int highScore, int combo);
void drawBars();
void drawBackground();
void resetBackGround();
void drawVignette();
void drawProgressBar();
void drawProgressBarI(bool interactable);
void drawButton(Rectangle rect, char * text, float fontScale);
void fMapSelect();
void drawMusicGraph(float transparent);
void drawTransition();
void initDrawing();

#endif