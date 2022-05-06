#ifndef GAME_DRAW_OBJECTS
#define GAME_DRAW_OBJECTS

#ifndef MAINC
#include "shared.h"
#include "include/raylib.h"
#include "files.h"
#include "gameplay.h"
#endif

Texture2D _cursorTex;
Texture2D _noteTex;
Texture2D _healthBarTex;
Texture2D _heartTex;
Texture2D _background;

Color _UIColor = WHITE;

struct Map;

typedef struct Map Map;

float noteFadeOut(float note);
void drawCursor();
void dNotes();
void drawMapThumbnail (Rectangle rect, Map *map);
void drawBars();
void drawVignette();
void drawProgressBar();
void drawButton(Rectangle rect, char * text, float fontScale);
void fMapSelect();

#endif