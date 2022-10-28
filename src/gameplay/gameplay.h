#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "audio.h"

#include "../deps/raylib/src/raylib.h"


#ifdef EXTERN_GAMEPLAY

	extern Map *_map;

	extern int _noteIndex, _amountNotes, _notesMissed;

	extern float _scrollSpeed, _maxMargin, _averageAccuracy;
	extern int _highScore, _score, _combo, _highestCombo, _highScoreCombo;
	extern bool _noBackground;

	extern Note ** _papNotes;

	extern Settings _settings;

	extern void (*_pNextGameplayFunction)(bool);
	extern void (*_pGameplayFunction)(bool);

	extern char _notfication[100];

#endif

typedef struct
{
	int id;
	float healthMod; // higher health mod means you die faster not slower
	float scoreMod;
	float speedMod;
	float marginMod;
	Texture *icon;
	char *name;
} Modifier;


void gotoMainMenu(bool mainOrSelect);
void checkFileDropped();
double getMusicHead();
bool isAnyKeyDown();

float getHealthMod();
float getScoreMod();
float getSpeedMod();
float getMarginMod();

#endif