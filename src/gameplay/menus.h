#ifndef GAME_MENUS
#define GAME_MENUS

#include "../deps/raylib/src/raylib.h"

#include "gameplay.h"

#ifdef EXTERN_MENUS

    extern Modifier *_activeMod[100];

    extern Modifier _mods[];

    extern bool _mapRefresh;

#endif


void fPause(bool reset);
bool mouseInRect(Rectangle rect);
void textBox(Rectangle rect, char *str, bool *selected);
void numberBox(Rectangle rect, int *number, bool *selected);
void slider(Rectangle rect, bool *selected, int *value, int max, int min);
bool drawInteractableButton(char *text, float fontScale, float x, float y, float width, float height);
bool interactableButton(char *text, float fontScale, float x, float y, float width, float height);
bool interactableButtonNoSprite(char *text, float fontScale, float x, float y, float width, float height);
void fMainMenu(bool reset);
void fSettings(bool reset);
void fNewMap(bool reset);
void fMapSelect(bool reset);
void fExport(bool reset);
void fIntro(bool reset);

#endif