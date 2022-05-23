#ifndef GAME_DRAW_OBJECTS
#define GAME_DRAW_OBJECTS

#include "shared.h"
#include "files.h"
#include "gameplay.h"

#include "../deps/raylib/src/raylib.h"

int measureText(char *text, int fontSize);
void drawText(char *str, int x, int y, int fontSize, Color color);
float musicTimeToScreen(float musicTime);
float screenToMusicTime(float x);
float noteFadeOut(float note);
void drawCursor();
void drawNote(float musicTime, Note * note, Color color);
void dNotes();
void drawMapThumbnail(Rectangle rect, Map *map, int highScore, int combo, float accuracy, bool selected);
void drawBars();
void drawBackground();
void resetBackGround();
void drawVignette();
void drawProgressBar();
bool drawProgressBarI(bool interactable);
void drawButton(Rectangle rect, char *text, float fontScale);
void drawButtonNoSprite(Rectangle rect, char *text, float fontScale);
void fMapSelect();
void drawMusicGraph(float transparent);
void drawTransition();
void initDrawing();
void drawLoadScreen();

#endif