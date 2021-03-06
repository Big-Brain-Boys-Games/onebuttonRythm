
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// #include "include/miniaudio.h"
#include "../deps/raylib/src/raylib.h"

#include "shared.h"
#include "files.h"
#include "drawing.h"
#include "gameplay.h"
#include "thread.h"

// #include "shared.c"
// #include "drawing.c"
// #include "gameplay.c"
// #include "files.c"

extern Texture2D _heartTex, _healthBarTex, _noteTex, _cursorTex, _background, _menuBackground;
extern void *_pEffectsBuffer, *_pHitSE, *_pMissHitSE, *_pMissSE;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size;
extern void (*_pGameplayFunction)();
extern Font _font;
extern bool _mapRefresh;

bool _disableLoadingScreen = false;
int _loading = 0;
float _loadingFade = 0;

bool _isKeyPressed = false;

float _transition = 0;

char _playerName[100];

#define GLSL_VERSION 330

int main(int argc, char **argv)
{
	LoadingMutexInit();
	initDrawing();
	audioInit();
	srand(time(NULL));
	snprintf(_playerName, 100, "guest%i", rand());
	loadSettings();

	initFolders();
	
	_pGameplayFunction = &fIntro;
	_transition = 1;
	float loadTimer = 0;
	while (!WindowShouldClose())
	{
		lockLoadingMutex();	
		_loadingFade += fmax(((_loading != 0 ? 1 : 0)-_loadingFade) * GetFrameTime()*15, -0.1);
		if(_loadingFade < 0)
			_loadingFade = 0;
		if (_loadingFade < _loading)
			_loadingFade = 1;
		unlockLoadingMutex();
		
		_isKeyPressed = isAnyKeyDown();
		if (_pGameplayFunction != &fMapSelect)
			_mapRefresh = true;
		
		BeginDrawing();
		if (IsKeyPressed(KEY_F11) || ((IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT)) && IsKeyPressed(KEY_ENTER)))
		{
			static int oldSizeX = 0;
			static int oldSizeY = 0;
			static int oldPosX = 0;
			static int oldPosY = 0;
			if (!IsWindowFullscreen())
			{
				oldSizeX = GetScreenWidth();
				oldSizeY = GetScreenHeight();
				oldPosX = GetWindowPosition().x;
				oldPosY = GetWindowPosition().y;
				SetWindowSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
				while (!IsWindowResized() && !WindowShouldClose())
				{
					BeginDrawing();
					drawLoadScreen();
					EndDrawing();
				}
				ToggleFullscreen();
			}
			else
			{
				ToggleFullscreen();
				SetWindowSize(oldSizeX, oldSizeY);
				while (!IsWindowResized() && !WindowShouldClose())
				{
					BeginDrawing();
					drawLoadScreen();
					EndDrawing();
				}
				SetWindowPosition(oldPosX, oldPosY);
			}
		}
		if (_loadingFade != 1 || _disableLoadingScreen)
		{
			if (_transition > 2)
				_transition = 0;
			if (_transition == 0 || _transition > 1)
				(*_pGameplayFunction)();

			if (_transition != 0)
			{
				_transition += GetFrameTime() * 7;
				drawTransition();
			}
		}
		if (_loadingFade != 0 && !_disableLoadingScreen)
		{
			drawLoadScreen();
		}
		// DrawFPS(0, 0);
		EndDrawing();
	}
	UnloadTexture(_background);
	CloseWindow();
	return 0;
}
