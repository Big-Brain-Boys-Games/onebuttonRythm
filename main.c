#define MINIAUDIO_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "include/miniaudio.h"
#include "include/raylib.h"

#define MAINC

#include "shared.h"
#include "files.h"
#include "drawing.h"
#include "gameplay.h"

#include "shared.c"
#include "drawing.c"
#include "gameplay.c"
#include "files.c"

#define GLSL_VERSION 330

#define EFFECT_BUFFER_SIZE 48000 * 4 * 4

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

	//todo do this smarter
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); //4 second long buffer
	ma_decoder tmp;
	_pHitSE = loadAudio("hit.mp3", &tmp, &_hitSE_Size);
	_pMissHitSE = loadAudio("missHit.mp3", &tmp, &_missHitSE_Size);
	_pMissSE = loadAudio("missHit.mp3", &tmp, &_missSE_Size);

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
