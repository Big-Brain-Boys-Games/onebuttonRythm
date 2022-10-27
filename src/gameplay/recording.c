#include <stdbool.h>
#include <stdio.h>

#include "../deps/raylib/src/raylib.h"

#include "menus.h"
#include "gameplay.h"
#include "editor.h"

#include "../drawing.h"
#include "../audio.h"
#include "../files.h"

extern void (*_pNextGameplayFunction)(bool);
extern void (*_pGameplayFunction)(bool);


extern double _musicHead;
extern bool _isKeyPressed;
extern int _amountNotes;

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
