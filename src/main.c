
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// #include "include/miniaudio.h"
#include "include/raylib.h"

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
float _transition = 0;

#define GLSL_VERSION 330


int main(int argc, char **argv)
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "One Button Rythm");
	SetTargetFPS(60);
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
	_transition = 1;
	while (!WindowShouldClose())
	{
		mousePos = GetMousePosition();
		BeginDrawing();
			if(_transition > 2)
				_transition = 0;
			if(_transition == 0 || _transition > 1)
				(*_pGameplayFunction)();
			
			if(_transition!=0)
			{
				_transition+=GetFrameTime()*7;
				drawTransition();
			}
		EndDrawing();
	}
	UnloadTexture(_background);
	CloseWindow();
	return 0;
}
