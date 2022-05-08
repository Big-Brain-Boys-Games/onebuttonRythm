
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// #include "include/miniaudio.h"
#include "deps/raylib/src/raylib.h"

#include "shared.h"
#include "files.h"
#include "drawing.h"
#include "gameplay.h"


// #include "shared.c"
// #include "drawing.c"
// #include "gameplay.c"
// #include "files.c"

extern Texture2D _heartTex, _healthBarTex, _noteTex, _cursorTex, _background, _menuBackground;
extern void *_pEffectsBuffer, *_pHitSE, *_pMissHitSE, *_pMissSE;
extern float * _pNotes;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size;
extern void (*_pGameplayFunction)();
extern Font _font;
extern bool _mapRefresh;

bool _isKeyPressed = false;

float _transition = 0;

#define GLSL_VERSION 330


int main(int argc, char **argv)
{
	
	initDrawing();
	Vector2 mousePos;
	audioInit();
	loadSettings();
	
	_pGameplayFunction = &fMainMenu;
	_transition = 1;
	_pNotes = malloc(sizeof(float)*50);
	while (!WindowShouldClose())
	{
		_isKeyPressed = isAnyKeyDown();
		if(_pGameplayFunction != &fMapSelect)
			_mapRefresh = true;
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
