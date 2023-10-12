#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

// #include "include/miniaudio.h"
#include "../deps/raylib/src/raylib.h"


#define EXTERN_GAMEPLAY
#define EXTERN_MENUS
#define EXTERN_DRAWING
#define EXTERN_AUDIO

#include "main.h"
#include "audio.h"
#include "files.h"
#include "shared.h"
#include "drawing.h"
#include "thread.h"



#include "gameplay/menus.h"
#include "gameplay/gameplay.h"


bool _disableLoadingScreen = false;
int _loading = 0;
float _loadingFade = 0;

bool _isKeyPressed = false;

float _transition = 0;

char _playerName[100];

int getWidth()
{
	return GetRenderWidth();
}

int getHeight()
{
	return GetRenderHeight();
}


void exitGame()
{
	_settings.fullscreen = IsWindowFullscreen();
	_settings.resolutionX = getWidth();
	_settings.resolutionY = getHeight();
	saveSettings();
	CloseWindow();
	exit(0);
}

#define GLSL_VERSION 330

int main(int argc, char **argv)
{
	SetTraceLogLevel(LOG_INFO || LOG_DEBUG || LOG_TRACE);
	snprintf(_playerName, 100, "guest%i", rand());
	loadSettings();
	LoadingMutexInit();
	initDrawing();
	audioInit();
	srand(time(NULL));
	SetRandomSeed(time(NULL));

	initFolders();
	
	_pGameplayFunction = &fIntro;
	_pNextGameplayFunction = &fMainMenu;
	_transition = 1;
	while (!WindowShouldClose())
	{
		lockLoadingMutex();	
		_loadingFade += fmax(((_loading != 0 ? 1 : 0)-_loadingFade) * GetFrameTime()*15, -0.1);
		if(_loadingFade < 0)
			_loadingFade = 0;
		if (_loadingFade < _loading)
			_loadingFade = 1;
		unlockLoadingMutex();

		//ugly workaround for audio.c needing both raylib and windows.h
		float globalVolume = _settings.volumeGlobal / 100.0;
		_musicVolume = _settings.volumeMusic / 100.0 * globalVolume;
		_audioEffectVolume = _settings.volumeSoundEffects / 100.0 * globalVolume;

		_scrollSpeed = (_wantedScrollSpeed * GetFrameTime()*15 + _scrollSpeed) / (1+GetFrameTime()*15);
		if(fabsf(1-_scrollSpeed/_wantedScrollSpeed)<0.01)
			_scrollSpeed = _wantedScrollSpeed;
		
		if(_musicPreviewTimer > 0 && _pGameplayFunction != fMapSelect)
		{
			_musicPreviewTimer = -1;
		}

		_isKeyPressed = isAnyKeyDown();
		if(IsKeyDown(KEY_ESCAPE))
			_isKeyPressed = false;
		
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
				ToggleFullscreen();
			}
			else
			{
				ToggleFullscreen();

				if(oldSizeX < 200)
					oldSizeX = 800;

				if(oldSizeY < 200)
					oldSizeY = 600;
				
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
				(*_pGameplayFunction)(false);

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
	exitGame();
	return 0;
}
