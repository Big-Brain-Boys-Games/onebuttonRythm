#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#include <stdbool.h>
#include "files.h"
#include "audio.h"

#include "../deps/raylib/src/raylib.h"


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
bool checkFileDropped();
double getMusicHead();
bool isAnyKeyDown();

float getHealthMod();
float getScoreMod();
float getSpeedMod();
float getMarginMod();

#endif