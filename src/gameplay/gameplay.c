#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>


#include "../../deps/raylib/src/raylib.h"


#define EXTERN_MAIN
#define EXTERN_MENUS
#define EXTERN_AUDIO

#include "gameplay.h"

#include "../drawing.h"
#include "../main.h"
#include "../files.h"
#include "../audio.h"

#include "playing.h"
#include "menus.h"



Map *_map;

int _noteIndex = 0, _amountNotes = 0, _notesMissed = 0;

float _scrollSpeed = 0.6, _maxMargin = 0.1, _averageAccuracy = 0, _highScoreAccuracy;
int _highScore, _score, _combo, _highestCombo, _highScoreCombo, _highScoreMisses;
bool _noBackground = false;

Note ** _papNotes = 0;

Settings _settings = (Settings){.volumeGlobal = 50, .volumeMusic = 100, .volumeSoundEffects = 100, .zoom = 7,
	.offset = -60, .noteSize=10, .resolutionX=800, .resolutionY=600, .fullscreen=0,
	.animations=1, .customAssets=1};

void (*_pNextGameplayFunction)(bool);
void (*_pGameplayFunction)(bool);

char _notfication[100];


float getHealthMod()
{
	if (_pGameplayFunction != &fPlaying)
		return 1;
	float value = 1;
	for (int i = 0; i < 100; i++)
		if (_activeMod[i] != 0)
			value *= _activeMod[i]->healthMod;
	return value;
}

float getScoreMod()
{
	if (_pGameplayFunction != &fPlaying)
		return 1;
	float value = 1;
	for (int i = 0; i < 100; i++)
		if (_activeMod[i] != 0)
			value *= _activeMod[i]->scoreMod;
	return value;
}

float getSpeedMod()
{
	if (_pGameplayFunction != &fPlaying)
		return 1;
	float value = 1;
	for (int i = 0; i < 100; i++)
		if (_activeMod[i] != 0)
			value *= _activeMod[i]->speedMod;
	return value;
}

float getMarginMod()
{
	if (_pGameplayFunction != &fPlaying)
		return 1;
	float value = 1;
	for (int i = 0; i < 100; i++)
		if (_activeMod[i] != 0)
			value *= _activeMod[i]->marginMod;
	return value;
}

void checkFileDropped()
{
	if (IsFileDropped())
	{
		FilePathList files = LoadDroppedFiles();
		for (int i = 0; i < files.count; i++)
		{
			if (strcmp(GetFileExtension(files.paths[i]), ".zip") == 0)
			{
				addZipMap(files.paths[i]);
				strncpy(_notfication, "map imported", 100);
			}
		}
		UnloadDroppedFiles(files);
	}
}

void gotoMainMenu(bool mainOrSelect)
{
	_disableLoadingScreen = false;
	stopMusic();
	_playMenuMusic = true;
	randomMusicPoint();
	if (mainOrSelect)
		_pGameplayFunction = &fMainMenu;
	else
		_pGameplayFunction = &fMapSelect;
	SetWindowTitle("One Button Rhythm");
	resetBackGround();
}

double getMusicHead()
{
	if (_musicPlaying)
		return _musicHead;
	else
		return _musicHead;
}

bool isAnyKeyDown()
{
	return GetKeyPressed() ||
		   IsMouseButtonPressed(0) ||
		   IsGamepadButtonPressed(0, GetGamepadButtonPressed()) ||
		   IsGamepadButtonPressed(1, GetGamepadButtonPressed()) ||
		   IsGamepadButtonPressed(2, GetGamepadButtonPressed()) ||
		   IsGamepadButtonPressed(3, GetGamepadButtonPressed());
}

