#define MINIAUDIO_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// #include "include/miniaudio.h"
#include "include/raylib.h"

// #define MAINC

#include "shared.h"
#include "files.h"
#include "drawing.h"
#include "gameplay.h"

// #include "shared.c"
// #include "drawing.c"
// #include "gameplay.c"
// #include "files.c"

extern Texture2D _heartTex, _healthBarTex, _noteTex, _cursorTex, _background;
extern void *_pEffectsBuffer, *_pHitSE, *_pMissHitSE, *_pMissSE;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size;
extern void (*_pGameplayFunction)();

#define GLSL_VERSION 330


int main(int argc, char **argv)
{
	InitWindow(800, 600, "One Button Rythm");
	SetTargetFPS(60);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetExitKey(0);

	HideCursor();

	_heartTex = LoadTexture("heart.png");
	_healthBarTex = LoadTexture("healthBar.png");
	_noteTex = LoadTexture("note.png");
	_cursorTex = LoadTexture("cursor.png");
	resetBackGround();
	
	Vector2 mousePos;
	audioInit();

	_pGameplayFunction = &fMainMenu;

	while (!WindowShouldClose())
	{
		mousePos = GetMousePosition();

		(*_pGameplayFunction)();
	}
	UnloadTexture(_background);
	CloseWindow();
	return 0;
}
