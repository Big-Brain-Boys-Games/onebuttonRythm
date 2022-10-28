#include <stdbool.h>
#include <stdio.h>

#include "../deps/raylib/src/raylib.h"


#define EXTERN_GAMEPLAY
#define EXTERN_AUDIO
#define EXTERN_MAIN

#include "recording.h"
#include "menus.h"
#include "gameplay.h"
#include "editor.h"

#include "../drawing.h"
#include "../audio.h"
#include "../files.h"
#include "../main.h"


void fRecording(bool reset)
{
	if (IsKeyPressed(KEY_ESCAPE))
	{
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fRecording;
		return;
	}
	_musicHead += GetFrameTime();
	fixMusicTime();

	ClearBackground(BLACK);
	drawBackground();

	if (_isKeyPressed && getMusicHead() >= 0)
	{
		printf("keyPressed! \n");

		newNote(getMusicHead());
		printf("music Time: %.2f\n", getMusicHead());
		ClearBackground(BLACK);
	}
	dNotes();
	drawVignette();
	drawBars();
	drawProgressBar();
	if (endOfMusic())
	{
		saveFile(_amountNotes);
		unloadMap();
		gotoMainMenu(true);
	}
}
