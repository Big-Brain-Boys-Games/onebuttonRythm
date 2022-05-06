#define MINIAUDIO_IMPLEMENTATION

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "include/miniaudio.h"
#include "include/raylib.h"

#include "shared.h"
#include "files.h"
#include "drawing.h"
#include "gameplay.h"

#include "drawing.c"
#include "gameplay.c"
#include "files.c"
#include "shared.c"

#define GLSL_VERSION 330

#define EFFECT_BUFFER_SIZE 48000 * 4 * 4

#define SUPPORT_FILEFORMAT_WAV
#define SUPPORT_FILEFORMAT_MP3
#define SUPPORT_FILEFORMAT_OGG

int main(int argc, char **argv)
{
	SetTargetFPS(60);
	// printf("size of int: %i", sizeof(int));
	// printf("size of float: %i", sizeof(float));
	// if(argc == 3) limit = strtol(argv[2], &p, 10);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Simple rythm game");

	HideCursor();

	_heartTex = LoadTexture("heart.png");
	_healthBarTex = LoadTexture("healthBar.png");
	_noteTex = LoadTexture("note.png");
	_cursorTex = LoadTexture("cursor.png");
	resetBackGround();

	// hitSE = LoadSound("hit.mp3");
	// SetSoundVolume(hitSE, 0.6f);
	// missHitSE = LoadSound("missHit.mp3");
	// SetSoundVolume(missHitSE, 1);
	// missSE = LoadSound("missSE.mp3");
	// SetSoundVolume(missSE, 1);

	Vector2 mousePos;

	//TODO do this smarter
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); // 4 second long buffer
	// for(int i = 0; i < EFFECT_BUFFER_SIZE/4; i++)
	// {
	// 	((_Float32*)_pEffectsBuffer)[i] = (_Float32)i;
	// }
	ma_decoder tmp;
	_pHitSE = loadAudio("hit.mp3", &tmp, &_hitSE_Size);
	_pMissHitSE = loadAudio("missHit.mp3", &tmp, &_missHitSE_Size);
	_pMissSE = loadAudio("missHit.mp3", &tmp, &_missSE_Size);

	_pGameplayFunction = &fMainMenu;
	// _pGameplayFunction = &fMapSelect;

	while (!WindowShouldClose())
	{
		mousePos = GetMousePosition();

		(*_pGameplayFunction)();
	}
	UnloadTexture(_background);
	CloseWindow();
	return 0;
}
