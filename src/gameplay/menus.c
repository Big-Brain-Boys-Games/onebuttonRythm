

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "../../deps/raylib/src/raylib.h"


#define EXTERN_MAIN
#define EXTERN_GAMEPLAY
#define EXTERN_DRAWING
#define EXTERN_AUDIO
#define EXTERN_FILES

#include "menus.h"

#include "gameplay.h"
#include "playing.h"
#include "recording.h"
#include "editor.h"

#include "../files.h"
#include "../drawing.h"
#include "../thread.h"
#include "../main.h"


Modifier *_activeMod[100] = {0}; // we dont even have that many

Modifier _mods[] = {
	(Modifier){.id = 0, .name = "No Fail", .icon = 0, .healthMod = 0, .scoreMod = 0.2, .speedMod = 1, .marginMod = 1},
	(Modifier){.id = 1, .name = "Easy", .icon = 0, .healthMod = 0.5, .scoreMod = 0.5, .speedMod = 1, .marginMod = 2},
	(Modifier){.id = 2, .name = "Hard", .icon = 0, .healthMod = 1.5, .scoreMod = 1.5, .speedMod = 1, .marginMod = 0.5},
	(Modifier){.id = 3, .name = ".5x", .icon = 0, .healthMod = 1, .scoreMod = 0.5, .speedMod = 0.5, .marginMod = 1},
	(Modifier){.id = 4, .name = "2x", .icon = 0, .healthMod = 1, .scoreMod = 1.5, .speedMod = 1.5 /* :P */, .marginMod = 1},
};

bool _mapRefresh = true;


void fMainMenu(bool reset)
{
	checkFileDropped();
	_musicLoops = true;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	int middle = getWidth() / 2;
	drawVignette();

	float fontScale = 0.075;
	//play button
	Rectangle button = (Rectangle){.x = getWidth() * 0.05, .y = getHeight() * 0.25, .width = getWidth() * 0.32, .height = getWidth() * 0.32};
	static float growTimer = 0;
	if(mouseInRect(button))
	{
		growTimer += GetFrameTime() * 12;
	}else
		growTimer -= GetFrameTime() * 12;

	growTimer = fmax(fmin(growTimer, 1), 0);
	
	button.width += getWidth()*0.1*growTimer;
	button.height += getWidth()*0.1*growTimer;
	button.x -= getWidth()*0.05*growTimer;
	button.y -= getWidth()*0.05*growTimer;

	DrawTexturePro(_noteTex, (Rectangle){.x=0, .y=0, .width=_noteTex.width, .height=_noteTex.height}, (Rectangle){.x=button.x, .y=button.y, .width=button.width, .height=button.width}, (Vector2) {.x=0, .y=0}, 0, WHITE);


	fontScale *= 1.3;
	int screenSize = getWidth() > getHeight() ? getHeight() : getWidth();
	int textSize = measureText("Play", screenSize * fontScale);
	drawText("Play", button.x + button.width / 2 - textSize / 2, button.y + button.height*0.5-getHeight()*0.045, screenSize * fontScale, DARKGRAY);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		playAudioEffect(_buttonSe);
		printf("switching to playing map!\n");
		_pNextGameplayFunction = &fPlaying;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if (interactableButton("Settings", 0.035, middle - getWidth() * (0.12-growTimer*0.03), getHeight() * 0.55, getWidth() * 0.2, getHeight() * 0.065))
	{
		// Switching to settings
		_pGameplayFunction = &fSettings;
		_transition = 0.1;
	}

	if (interactableButton("New Map", 0.035, middle - getWidth() * (0.03 - growTimer*0.03), getHeight() * 0.65, getWidth() * 0.2, getHeight() * 0.065))
	{
		_pGameplayFunction = &fNewMap;
		_transition = 0.1;
	}

	if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Exit", 0.035, middle - getWidth() * (-0.07 - growTimer*0.03), getHeight() * 0.75, getWidth() * 0.2, getHeight() * 0.065))
	{
		exitGame(0);
	}

	// gigantic title
	float tSize = getWidth() * 0.07;
	Vector2 titlePos = (Vector2){.x=getWidth()*0.5, .y=getHeight()*0.2};
	// dropshadow
	drawText("One Button", titlePos.x + getWidth() * 0.004, titlePos.y + getHeight()* 0.007, tSize, ColorAlpha(BLACK, 0.4));
	// real title
	drawText("One Button", titlePos.x, titlePos.y, tSize, WHITE);

	titlePos.x += getWidth()*0.07;
	titlePos.y += getHeight()*0.1;
	// dropshadow
	drawText("Rhythm", titlePos.x + getWidth() * 0.004, titlePos.y + getHeight()* 0.007, tSize, ColorAlpha(BLACK, 0.4));
	// real title
	drawText("Rhythm", titlePos.x, titlePos.y, tSize, WHITE);

	char str[120];
	snprintf(str, 120, "name: %s", _playerName);
	drawText(str, getWidth() * 0.55, getHeight() * 0.92, getWidth() * 0.04, WHITE);

	drawText(_notfication, getWidth() * 0.6, getHeight() * 0.8, getWidth() * 0.02, WHITE);

	drawCursor();
}

void fSettings(bool reset)
{

	static float menuScroll = 0;
	static float menuScrollSmooth = 0;
	if(reset)
	{
		menuScroll = 0;
		menuScrollSmooth = 0;
		return;
	}
	
	_musicPlaying = false;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);
	int middle = getWidth() / 2;

	DrawRectangle(0, 0, getWidth(), getHeight()*0.13, ColorAlpha(BLACK ,0.4));
	// gigantic ass settings title
	char *title = "Settings";
	float tSize = getWidth() * 0.05;
	int size = MeasureText(title, tSize);
	// dropshadow
	drawText(title, middle - size / 2 + getWidth() * 0.004, getHeight() * 0.03, tSize, DARKGRAY);
	// real title
	drawText(title, middle - size / 2, getHeight() * 0.02, tSize, WHITE);

	
	menuScroll += GetMouseWheelMove() * .04;
	menuScrollSmooth += (menuScroll - menuScrollSmooth) * GetFrameTime() * 15;
	if (IsMouseButtonDown(0))
	{ // scroll by dragging
		menuScroll += GetMouseDelta().y / getHeight();
	}

	menuScroll = (int)fmin(fmax(menuScroll, 0), 1);

	Rectangle settingsRect = (Rectangle){.x=0, .y=getHeight()*0.13, .width=getWidth(), .height=getHeight()};
	BeginScissorMode(settingsRect.x, settingsRect.y, settingsRect.width, settingsRect.height);

		char zoom[10] = {0};
		if (_settings.zoom != 0)
			snprintf(zoom, 10, "%i", _settings.zoom);
		static bool zoomBoxSelected = false;

		Rectangle zoomBox = (Rectangle){.x = getWidth() * 0.1, .y = getHeight() * (0.7+menuScrollSmooth), .width = getWidth() * 0.2, .height = getHeight() * 0.07};
		textBox(zoomBox, zoom, &zoomBoxSelected);

		if(!mouseInRect(settingsRect))
			zoomBoxSelected = false;

		_settings.zoom = atoi(zoom);
		_settings.zoom = fmin(fmax(_settings.zoom, 0), 300);
		tSize = getWidth() * 0.03;
		size = MeasureText("zoom", tSize);
		drawText("zoom", zoomBox.x + zoomBox.width / 2 - size / 2, zoomBox.y - getHeight() * 0.05, tSize, WHITE);


		
		char noteSize[10] = {0};
		if (_settings.noteSize != 0)
			snprintf(noteSize, 10, "%i", _settings.noteSize);
		static bool noteSizeBoxSelected = false;

		Rectangle noteSizeBox = (Rectangle){.x = getWidth() * 0.52, .y = getHeight() * (0.5+menuScrollSmooth), .width = getWidth() * 0.2, .height = getHeight() * 0.07};
		textBox(noteSizeBox, noteSize, &noteSizeBoxSelected);

		if(!mouseInRect(settingsRect))
			noteSizeBoxSelected = false;

		_settings.noteSize = atoi(noteSize);
		_settings.noteSize = fmin(fmax(_settings.noteSize, 0), 20);
		tSize = getWidth() * 0.03;
		size = MeasureText("noteSize", tSize);
		drawText("noteSize", noteSizeBox.x + noteSizeBox.width / 2 - size / 2, noteSizeBox.y - getHeight() * 0.05, tSize, WHITE);






		static bool nameBoxSelected = false;
		Rectangle nameBox = (Rectangle){.x = getWidth() * 0.52, .y = getHeight() * (0.3+menuScrollSmooth), .width = getWidth() * 0.3, .height = getHeight() * 0.07};
		textBox(nameBox, _playerName, &nameBoxSelected);

		char offset[10] = {0};
		if (_settings.offset != 0)
			snprintf(offset, 10, "%i", (int)(_settings.offset*1000));
		static bool offsetBoxSelected = false;
		Rectangle offsetBox = (Rectangle){.x = getWidth() * 0.1, .y = getHeight() * (0.85+menuScrollSmooth), .width = getWidth() * 0.2, .height = getHeight() * 0.07};
		textBox(offsetBox, offset, &offsetBoxSelected);
		_settings.offset = (float)atoi(offset);
		_settings.offset = fmin(fmax(_settings.offset, -300), 300);
		_settings.offset *= 0.001;

		if(offsetBoxSelected && IsKeyPressed(KEY_MINUS))
		{
			_settings.offset *= -1;
		}
		

		static bool offsetSliderSelected = false;
		Rectangle offsetSlider = (Rectangle){.x = getWidth() * 0.1, .y = getHeight() * (0.9+menuScrollSmooth), .width = getWidth() * 0.2, .height = getHeight() * 0.03};
		int tempOffset = _settings.offset * 1000;
		slider(offsetSlider, &offsetSliderSelected, &tempOffset, 300, -300);
		_settings.offset = tempOffset * 0.001;

		tSize = getWidth() * 0.03;
		size = MeasureText("offset", tSize);
		drawText("offset", offsetBox.x + offsetBox.width / 2 - size / 2, offsetBox.y - getHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			offsetBoxSelected = false;

		static bool gvBoolSelected = false;
		Rectangle gvSlider = (Rectangle){.x = getWidth() * 0.05, .y = getHeight() * (0.3+menuScrollSmooth), .width = getWidth() * 0.3, .height = getHeight() * 0.03};
		slider(gvSlider, &gvBoolSelected, &_settings.volumeGlobal, 100, 0);
		tSize = getWidth() * 0.03;
		size = MeasureText("global volume", tSize);
		drawText("global volume", gvSlider.x + gvSlider.width / 2 - size / 2, gvSlider.y - getHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			gvBoolSelected = false;

		static bool mvBoolSelected = false;
		Rectangle mvSlider = (Rectangle){.x = getWidth() * 0.05, .y = getHeight() * (0.45+menuScrollSmooth), .width = getWidth() * 0.3, .height = getHeight() * 0.03};
		slider(mvSlider, &mvBoolSelected, &_settings.volumeMusic, 100, 0);
		tSize = getWidth() * 0.03;
		size = MeasureText("music volume", tSize);
		drawText("music volume", mvSlider.x + mvSlider.width / 2 - size / 2, mvSlider.y - getHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			mvBoolSelected = false;


		static bool aevBoolSelected = false;
		Rectangle aevSlider = (Rectangle){.x = getWidth() * 0.05, .y = getHeight() * (0.6+menuScrollSmooth), .width = getWidth() * 0.3, .height = getHeight() * 0.03};
		slider(aevSlider, &aevBoolSelected, &_settings.volumeSoundEffects, 100, 0);
		tSize = getWidth() * 0.03;
		size = MeasureText("sound sffect volume", tSize);
		drawText("sound Effect volume", aevSlider.x + aevSlider.width / 2 - size / 2, aevSlider.y - getHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			aevBoolSelected = false;

		drawVignette();
	EndScissorMode();

	if ( IsKeyPressed(KEY_ESCAPE) || interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
		
		if(_settings.noteSize == 0)
			_settings.noteSize = 10;
		
		saveSettings();
		return;
	}

	drawCursor();
}


void fPause(bool reset)
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();

	dNotes();
	drawVignette();
	drawProgressBar();
	DrawRectangle(0, 0, getWidth(), getHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	// TODO dynamically change seperation depending on the amount of buttons?
	float middle = getWidth() / 2;

	if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Continue", 0.05, middle - getWidth() * 0.15, getHeight() * 0.3, getWidth() * 0.3, getHeight() * 0.1))
	{
		if (_pNextGameplayFunction == &fPlaying)
		{
			_pGameplayFunction = &fCountDown;
			fCountDown(true);
		}
		else
			_pGameplayFunction = _pNextGameplayFunction;
	}
	if (_pNextGameplayFunction == &fEditor && interactableButton("Save", 0.05, middle - getWidth() * 0.15, getHeight() * 0.5, getWidth() * 0.3, getHeight() * 0.1))
	{
		saveMap();
		_pGameplayFunction = _pNextGameplayFunction;
		// gotoMainMenu(false);
	}

	if (_pNextGameplayFunction == &fPlaying && interactableButton("retry", 0.05, middle - getWidth() * 0.15, getHeight() * 0.5, getWidth() * 0.3, getHeight() * 0.1))
	{
		_pGameplayFunction = fCountDown;
		_noteIndex = 0;
		_musicHead = 0;
		fCountDown(true);
		fPlaying(true);
	}

	if (_pNextGameplayFunction == &fRecording && interactableButton("retry", 0.05, middle - getWidth() * 0.15, getHeight() * 0.5, getWidth() * 0.3, getHeight() * 0.1))
	{
		_pGameplayFunction = _pNextGameplayFunction;
		_noteIndex = 0;
		_amountNotes = 0;
		_musicHead = 0;
		fRecording(true);
	}

	if (interactableButton("Exit", 0.05, middle - getWidth() * 0.15, getHeight() * 0.7, getWidth() * 0.3, getHeight() * 0.1))
	{
		unloadMap();
		gotoMainMenu(false);
	}
	drawCursor();
}




struct mapInfoLoadingArgs
{
	int *amount;
	int **highScores;
	int **combos;
	float **accuracy;
};

char **filesCaching = 0;

#ifdef __unix
void mapInfoLoading(struct mapInfoLoadingArgs *args)
#else
DWORD WINAPI *mapInfoLoading(struct mapInfoLoadingArgs *args)
#endif
{
	lockLoadingMutex();
	_loading++;
	unlockLoadingMutex();
	static int oldAmount = 0;
	FilePathList files = LoadDirectoryFiles("maps/");

	int amount = 0;
	for(int i = 0; i < files.count; i++)
	{
		char str [100];
		snprintf(str, 100, "%s/map.data", files.paths[i]);
		if(FileExists(str))
		{
			amount++;
		}
	}
	
	char **newFiles = malloc(sizeof(char *) * amount);
	for(int i = 0, j = 0; i < files.count && j < amount; i++)
	{
		char str [100];
		snprintf(str, 100, "%s/map.data", files.paths[i]);
		if(FileExists(str))
		{
			newFiles[j] = malloc(sizeof(char) * (strlen(files.paths[i])+1));
			strcpy(newFiles[j], files.paths[i]);
			j++;
		}
	}
	UnloadDirectoryFiles(files);

	if (args->highScores != 0)
	{
		*args->highScores = realloc(*args->highScores, amount * sizeof(int));
		*args->combos = realloc(*args->combos, amount * sizeof(int));
		*args->accuracy = realloc(*args->accuracy, amount * sizeof(float));
		char **tmp = calloc(amount, sizeof(char *));
		for (int i = 0; i < amount && i < oldAmount; i++)
		{
			tmp[i] = filesCaching[i];
		}
		for (int i = oldAmount; i < amount; i++)
			tmp[i] = calloc(100, sizeof(char));
		for (int i = amount; i < oldAmount; i++)
			free(filesCaching[i]);
		free(filesCaching);
		filesCaching = tmp;

		Map *tmp2 = calloc(amount, sizeof(Map));
		for (int i = 0; i < amount && i < oldAmount; i++)
		{
			tmp2[i] = _paMaps[i];
		}
		for (int i = amount; i < oldAmount; i++)
			freeMap(&_paMaps[i]);
		free(_paMaps);
		_paMaps = tmp2;
	}
	else
	{
		*args->highScores = malloc(amount * sizeof(int));
		*args->combos = malloc(amount * sizeof(int));
		*args->accuracy = malloc(amount * sizeof(float));
		filesCaching = calloc(amount, sizeof(char *));
		for (int i = 0; i < amount; i++)
			filesCaching[i] = calloc(100, sizeof(char));
		_paMaps = calloc(amount, sizeof(Map));
	}
	oldAmount = amount;
	int mapIndex = 0;
	for (int i = 0; i < amount; i++)
	{
		if (newFiles[i][0] == '.')
			continue;
		// check for cache
		bool cacheHit = false;
		for (int j = 0; j < amount; j++)
		{
			if (!filesCaching[j][0])
				continue;
			if (strncmp(filesCaching[j], newFiles[i], 100) == 0)
			{
				// cache hit
				printf("cache hit! %s\n", newFiles[i]);
				cacheHit = true;
				if (mapIndex == j)
				{
					break;
				}
				strcpy(filesCaching[mapIndex], filesCaching[j]);
				_paMaps[mapIndex] = _paMaps[j];
				_paMaps[j] = (Map){0};
				readScore(&_paMaps[mapIndex],
						  &((*args->highScores)[mapIndex]),
						  &((*args->combos)[mapIndex]),
						  &((*args->accuracy)[mapIndex]));
				break;
			}
		}
		if (cacheHit)
		{
			mapIndex++;
			continue;
		}
		printf("cache miss %s %i\n", newFiles[i], mapIndex);
		// cache miss

		freeMap(&_paMaps[mapIndex]);
		_paMaps[mapIndex] = loadMapInfo(newFiles[i]);
		if(_paMaps[mapIndex].name == 0)
		{
			printf("skipping map, failed to load\n");
			continue;
		}
		if (_paMaps[mapIndex].name != 0)
		{
			readScore(&_paMaps[mapIndex],
					  &((*args->highScores)[mapIndex]),
					  &((*args->combos)[mapIndex]),
					  &((*args->accuracy)[mapIndex]));
		}

		// caching
		strcpy(filesCaching[mapIndex], newFiles[i]);

		mapIndex++;
	}
	lockLoadingMutex();
	_loading--;
	unlockLoadingMutex();
	*args->amount = mapIndex;
	free(args);
}

#ifdef __unix
void loadMapImage(Map *map)
#else
DWORD WINAPI *loadMapImage(Map * map)
#endif
{
	printf("loading map image %s\n", map->imageFile);

	//disabled loading lock because some images refuse to load
	// lockLoadingMutex();
	// _loading++;
	// unlockLoadingMutex();
	Image img ={0};
	char str [100];
	snprintf(str, 100, "%s/%s", map->folder, map->imageFile);
	img = LoadImage(str);
	map->cpuImage = img;
	if(img.width==0) //failed to load so setting it back to -1
		map->cpuImage.width=-2;
	// lockLoadingMutex();
	// _loading--;
	// unlockLoadingMutex();
}

#include <unistd.h>

void fMapSelect(bool reset)
{
	static int amount = 0;
	static int *highScores;
	static int *combos;
	static float *accuracy;
	static int selectedMap = -1;
	static float selectMapTransition = 1;
	static int hoverMap = -1;
	static float hoverPeriod = 0;
	static bool selectingMods = false;
	_musicSpeed = 1;
	static char search[100];
	static bool searchSelected;

	if (selectMapTransition < 1)
		selectMapTransition += GetFrameTime() * 10;
	if (selectMapTransition > 1)
		selectMapTransition = 1;
	checkFileDropped();
	if (_mapRefresh)
	{
		struct mapInfoLoadingArgs *args = malloc(sizeof(struct mapInfoLoadingArgs));
		args->amount = &amount;
		args->combos = &combos;
		args->highScores = &highScores;
		args->accuracy = &accuracy;
		createThread((void *(*)(void *))mapInfoLoading, args);
		_mapRefresh = false;
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	if (selectingMods)
	{
		_playMenuMusic = true;
		_musicPlaying = false;
		hoverPeriod = 0;
		drawVignette();
		if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
		{
			selectingMods = false;
			return;
		}
		// draw all modifiers
		int amountMods = sizeof(_mods) / sizeof(_mods[0]);
		for (int i = 0; i < amountMods; i++)
		{
			int x = i % 3;
			int y = i / 3;
			if (interactableButton(_mods[i].name, 0.03, getWidth() * (0.2 + x * 0.22), getHeight() * (0.6 + y * 0.12), getWidth() * 0.2, getHeight() * 0.07))
			{
				// enable mod
				int modId = _mods[i].id;
				bool foundDouble = false;
				for (int j = 0; j < amountMods; j++)
				{
					// find doubles
					if (_activeMod[j] != NULL && modId == _activeMod[j]->id)
					{
						foundDouble = true;
						// disable mod
						_activeMod[j] = 0;
						break;
					}
				}
				if (!foundDouble)
				{
					// add new mod
					// find empty spot
					for (int j = 0; j < 100; j++)
					{
						if (_activeMod[j] == 0)
						{
							_activeMod[j] = &(_mods[i]);
							break;
						}
					}
				}
			}
		}

		// print selected / active mods
		int modsSoFar = 0;
		for (int i = 0; i < 100; i++)
		{
			if (_activeMod[i] != 0)
			{
				drawText(_activeMod[i]->name, getWidth() * (0.05 + 0.12 * modsSoFar), getHeight() * 0.9, getWidth() * 0.03, WHITE);
				modsSoFar++;
			}
		}

		drawCursor();
		return;
	}
	DrawRectangle(0, 0, getWidth(), getHeight()*0.13, BLACK);
	static float menuScroll = 0;
	static float menuScrollSmooth = 0;
	menuScroll += GetMouseWheelMove() * .04;
	menuScrollSmooth += (menuScroll - menuScrollSmooth) * GetFrameTime() * 15;
	if (IsMouseButtonDown(0))
	{ // scroll by dragging
		menuScroll += GetMouseDelta().y / getHeight();
	}
	const float scrollSpeed = .03;
	if (IsKeyDown(KEY_UP))
	{
		menuScroll += scrollSpeed;
	}
	if (IsKeyDown(KEY_DOWN))
	{
		menuScroll -= scrollSpeed;
	}
	menuScroll = clamp(menuScroll, -.5 * floor(amount / 2), 0);

	if (interactableButton("Mods", 0.03, getWidth() * 0.2, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		selectingMods = true;
	}

	if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
	}

	textBox((Rectangle){.x = getWidth() * 0.35, .y = getHeight() * 0.05, .width = getWidth() * 0.2, .height = getHeight() * 0.05}, search, &searchSelected);

	if (hoverMap == -1)
	{
		_musicFrameCount = 1;
		hoverPeriod = 0;
	}
	else
	{
		_disableLoadingScreen = true;
		hoverPeriod += GetFrameTime();
		// if(_musicLength)
		// 	if (!*_musicLength)
		// 		_musicFrameCount = _paMaps[hoverMap].musicPreviewOffset * 48000 * 2;
	}
	// draw map button
	Rectangle mapSelectRect = (Rectangle){.x = 0, .y = getHeight() * 0.13, .width = getWidth(), .height = getHeight()};
	BeginScissorMode(mapSelectRect.x, mapSelectRect.y, mapSelectRect.width, mapSelectRect.height);
	int mapCount = -1;
	for (int i = 0; i < amount; i++)
	{
		// draw maps
		if (search[0] != '\0')
		{
			bool missingLetter = false;
			char str[100];
			snprintf(str, 100, "%s - %s", _paMaps[i].name, _paMaps[i].artist);
			int stringLength = strlen(str);
			for (int j = 0; j < 100 && search[j] != '\0'; j++)
			{
				bool foundOne = false;
				for (int k = 0; k < stringLength; k++)
				{
					if (tolower(search[j]) == tolower(str[k]))
						foundOne = true;
				}
				if (!foundOne)
				{
					missingLetter = true;
					break;
				}
			}
			if (missingLetter)
				continue;
		}

		mapCount++;
		int x = getWidth() * 0.02 + getWidth() * 0.32 * (mapCount % 3);
		Rectangle mapButton = (Rectangle){.x = x, .y = menuScrollSmooth * getHeight() + getHeight() * ((floor(i / 3) > floor(selectedMap / 3) && selectedMap != -1 ? 0.3 : 0.225) + 0.3375 * floor(mapCount / 3)), .width = getWidth() * 0.3, .height = getHeight() * 0.3};
		if ((mouseInRect(mapButton) || selectedMap == i) && mouseInRect(mapSelectRect))
		{
			if (hoverPeriod > 1 && hoverPeriod < 2)
			{
				// play music
				char str[100];
				snprintf(str, 100, "%s/%s", _paMaps[i].folder, _paMaps[i].musicFile);
				loadMusic(&_paMaps[i].music, str, _paMaps[i].musicPreviewOffset);
				_playMenuMusic = false;
				_musicFrameCount = _paMaps[i].musicPreviewOffset * 48000 * 2;
				_musicPlaying = true;
				hoverPeriod++;
			}
			hoverMap = i;
			_disableLoadingScreen = true;
		}
		else if ((!mouseInRect(mapButton) || !mouseInRect(mapSelectRect)) && hoverMap == i)
		{
			hoverMap = -1;
			_playMenuMusic = true;
			_musicPlaying = false;
			hoverPeriod = 0;
			_musicFrameCount = 1;
		}

		if(_paMaps[i].cpuImage.width == 0)
		{
			_paMaps[i].cpuImage.width = -1;
			createThread((void *(*)(void *))loadMapImage, &_paMaps[i]);
			_paMaps[i].cpuImage.width = -1;
		}
		if (selectedMap == i)
		{
			if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
				selectedMap = -1;

			Rectangle buttons = (Rectangle){.x = mapButton.x, .y = mapButton.y + mapButton.height, .width = mapButton.width, .height = mapButton.height * 0.15 * selectMapTransition};
			if (mouseInRect(buttons) && IsMouseButtonReleased(0) && mouseInRect(mapSelectRect))
			{
				_map = &_paMaps[i];
				setMusicStart();
				_musicHead = 0;
				printf("selected map!\n");
				_transition = 0.1;
				_disableLoadingScreen = false;
				_musicPlaying = false;
				//wait until map image is loaded
				// if(_pGameplayFunction != &fMapSelect)
				// {
				// 	float startTime = (float)clock()/CLOCKS_PER_SEC;
				// 	for(int i = 0; i < 20 && _map->cpuImage.width < 1; i++)
				// 	{
				// 		// #ifdef _WIN32
				// 		// 	Sleep(100);
				// 		// #else
				// 		// 	usleep(100);
				// 		// #endif
				// 	}

				// 	if(_map->cpuImage.width < 1)
				// 		_map->image = _background;
				// }
			}
			drawMapThumbnail(mapButton, &_paMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], true);
			if (interactableButtonNoSprite("play", 0.0225, mapButton.x, mapButton.y + mapButton.height, mapButton.width * (1 / 3.0) * 1.01, mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect))
			{
				_pNextGameplayFunction = &fPlaying;
				_pGameplayFunction = &fCountDown;
				fCountDown(true);
				fPlaying(true);
			}
			if (interactableButtonNoSprite("editor", 0.0225, mapButton.x + mapButton.width * (1 / 3.0), mapButton.y + mapButton.height, mapButton.width * (1 / 3.0) * 1.01, mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect))
			{
				_pNextGameplayFunction = &fEditor;
				_pGameplayFunction = &fEditor;
				fEditor(true);
			}
			if (interactableButtonNoSprite("export", 0.0225, mapButton.x + mapButton.width * (1 / 3.0 * 2), mapButton.y + mapButton.height, mapButton.width * (1 / 3.0), mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect))
			{
				loadMap();
				_pGameplayFunction = &fExport;
			}
			DrawRectangleGradientV(mapButton.x, mapButton.y + mapButton.height, mapButton.width, mapButton.height * 0.05 * selectMapTransition, ColorAlpha(BLACK, 0.3), ColorAlpha(BLACK, 0));
		}
		else
		{
			drawMapThumbnail(mapButton, &_paMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], false);

			if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
			{
				playAudioEffect(_buttonSe);
				selectedMap = i;
				selectMapTransition = 0;
				hoverPeriod = 0;
				_musicFrameCount = 1;
			}
		}
	}
	
	drawVignette();

	EndScissorMode();

	if (hoverMap != -1 || selectedMap != -1)
	{
		int selMap = selectedMap != -1 ? selectedMap : hoverMap;
		if(_paMaps[selMap].name != 0)
		{
			char str[100];
			strcpy(str, _paMaps[selMap].name);
			strcat(str, " - ");
			strcat(str, _paMaps[selMap].artist);
			int textSize = measureText(str, getWidth() * 0.05);
			drawText(str, getWidth() * 0.9 - textSize, getHeight() * 0.92, getWidth() * 0.05, WHITE);
		}
	}

	drawCursor();
}



void fExport(bool reset)
{
	_pGameplayFunction = &fMainMenu;
	makeMapZip(_map);
	char str[300];
	strcpy(str, GetWorkingDirectory());
	strcat(str, "/");
	strcat(str, _map->name);
	strcat(str, ".zip");
	SetClipboardText(str);
	resetBackGround();
	strcpy(_notfication, "exported map");
}

void fNewMap(bool reset)
{
	static Map newMap = {0};
	static void *pMusic = 0;
	static char pMusicExt[50];
	static int pMusicSize = 0;
	static void *pImage = 0;
	static int imageSize = 0;

	if (newMap.name == 0)
	{
		newMap.name = malloc(100);
		strcpy(newMap.name, "name");
		newMap.artist = malloc(100);
		newMap.artist[0] = '\0';
		strcpy(newMap.artist, "Artist");
		newMap.mapCreator = malloc(100);
		newMap.mapCreator[0] = '\0';
		strcpy(newMap.mapCreator, "mapCreator");
		newMap.folder = malloc(100);
		newMap.folder[0] = '\0';
		newMap.bpm = 100;
		newMap.beats = 4;
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	int middle = getWidth() / 2;

	if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
		return;
	}

	if (interactableButton("Finish", 0.02, getWidth() * 0.85, getHeight() * 0.85, getWidth() * 0.1, getHeight() * 0.05))
	{
		if (pMusic == 0)
			return;
		if (pImage == 0)
			return;
		makeMap(&newMap);
		char str[100];
		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/song");
		strcat(str, pMusicExt);
		FILE *file = fopen(str, "wb");
		fwrite(pMusic, pMusicSize, 1, file);
		fclose(file);

		newMap.musicFile = malloc(100);
		snprintf(newMap.musicFile, 100, "/song%s", pMusicExt);

		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/image.png");
		file = fopen(str, "wb");
		fwrite(pImage, imageSize, 1, file);
		fclose(file);
		if (newMap.bpm == 0)
			newMap.bpm = 1;
		_pNextGameplayFunction = &fEditor;
		_amountNotes = 0;
		_noteIndex = 0;
		_pGameplayFunction = &fEditor;
		_transition = 0.1;
		newMap.folder = malloc(100);
		strcpy(newMap.folder, newMap.name);
		_map = &newMap;
		freeNotes();
		saveMap();
		printf("map : %s\n", newMap.name);
		loadMap();
		_noBackground = 0;
		setMusicStart();
		_musicHead = 0;
		_transition = 0.1;
		_disableLoadingScreen = false;
		startMusic();
		return;
	}

	// text boxes
	static bool nameBoxSelected = false;
	Rectangle nameBox = (Rectangle){.x = middle, .y = getHeight() * 0.375, .width = getWidth() * 0.3, .height = getHeight() * 0.07};
	textBox(nameBox, newMap.name, &nameBoxSelected);
	drawText("name", middle, getHeight() * 0.325, getHeight() * 0.05, WHITE);

	static bool artistBoxSelected = false;
	Rectangle artistBox = (Rectangle){.x = middle, .y = getHeight() * 0.5, .width = getWidth() * 0.3, .height = getHeight() * 0.07};
	textBox(artistBox, newMap.artist, &artistBoxSelected);
	drawText("artist", middle, getHeight() * 0.45, getHeight() * 0.05, WHITE);

	static bool creatorBoxSelected = false;
	Rectangle creatorBox = (Rectangle){.x = middle, .y = getHeight() * 0.625, .width = getWidth() * 0.3, .height = getHeight() * 0.07};
	textBox(creatorBox, newMap.mapCreator, &creatorBoxSelected);
	drawText("creator", middle, getHeight() * 0.575, getHeight() * 0.05, WHITE);

	char str[100] = {'\0'};
	if (newMap.bpm != 0)
		snprintf(str, 100, "%i", newMap.bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x = middle, .y = getHeight() * 0.875, .width = getWidth() * 0.3, .height = getHeight() * 0.07};
	textBox(bpmBox, str, &bpmBoxSelected);
	newMap.bpm = fmin(fmax(atoi(str), 0), 500);
	drawText("bpm", middle, getHeight() * 0.825, getHeight() * 0.05, WHITE);

	static bool difficultyBoxSelected = false;
	Rectangle difficultyBox = (Rectangle){.x = middle, .y = getHeight() * 0.75, .width = getWidth() * 0.3, .height = getHeight() * 0.07};
	numberBox(difficultyBox, &newMap.difficulty, &difficultyBoxSelected);
	if (newMap.difficulty < 0)
		newMap.difficulty = 0;
	if (newMap.difficulty > 9)
		newMap.difficulty = 0;
	drawText("difficulty", middle, getHeight() * 0.70, getHeight() * 0.05, WHITE);

	int textSize = measureText("Drop in .png, .wav or .mp3", getWidth() * 0.04);
	drawText("Drop in .png, .wav or .mp3", getWidth() * 0.5 - textSize / 2, getHeight() * 0.2, getWidth() * 0.04, WHITE);

	textSize = measureText("missing music file", getWidth() * 0.03);
	if (pMusic == 0)
		drawText("missing music file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.6, getWidth() * 0.03, WHITE);
	else
		drawText("got music file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.6, getWidth() * 0.03, WHITE);

	textSize = measureText("missing image file", getWidth() * 0.03);
	if (pImage == 0)
		drawText("missing image file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.7, getWidth() * 0.03, WHITE);
	else
		drawText("got image file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.7, getWidth() * 0.03, WHITE);

	drawCursor();

	// file dropping
	if (IsFileDropped() || ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))))
	{
		int amount = 0;
		char **files;
		FilePathList filePaths;
		bool keyOrDrop = true;
		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			// copy paste
			char *str = (char *) GetClipboardText();

			int file = 0;
			int partIndex = 0;
			files = malloc(strlen(str));
			char * part = calloc( strlen(str), sizeof(char));
			for (int i = 0; str[i] != '\0'; i++)
			{
				part[partIndex++] = str[i];
				if (str[i + 1] == '\n' || str[i + 1] == '\0')
				{
					i++;
					part[partIndex + 1] = '\0';
					printf("file %i: %s\n", file, part);
					if (FileExists(part))
					{
						printf("\tfile exists\n");
						files[file] = malloc(partIndex);
						strcpy(files[file], part);
						file++;
					}
					partIndex = 0;
				}
			}
			amount = file;
			keyOrDrop = false;
		}
		else
		{
			filePaths = LoadDroppedFiles();
			files = filePaths.paths;
			amount = filePaths.count;
		}

		for (int i = 0; i < amount; i++)
		{
			const char *ext = GetFileExtension(files[i]);
			if (strcmp(ext, ".png") == 0)
			{
				if (newMap.image.id != 0)
					UnloadTexture(newMap.image);
				newMap.image = LoadTexture(files[i]);

				if (pImage != 0)
					free(pImage);
				FILE *file = fopen(files[i], "rb");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pImage = malloc(size);
				fread(pImage, size, 1, file);
				fclose(file);
				imageSize = size;
			}

			if (strcmp(ext, ".mp3") == 0)
			{
				if (pMusic != 0)
					free(pMusic);
				FILE *file = fopen(files[i], "rb");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pMusic = malloc(size);
				fread(pMusic, size, 1, file);
				fclose(file);
				strcpy(pMusicExt, ext);
				pMusicSize = size;
			}

			if (strcmp(ext, ".wav") == 0)
			{
				if (pMusic != 0)
					free(pMusic);
				FILE *file = fopen(files[i], "rb");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pMusic = malloc(size);
				fread(pMusic, size, 1, file);
				strcpy(pMusicExt, ext);
				fclose(file);
				pMusicSize = size;
			}
		}
		if (!keyOrDrop)
		{
			for (int i = 0; i < amount; i++)
				free(files[i]);
			free(files);
		}
		else
			UnloadDroppedFiles(filePaths);
	}
}

void fIntro(bool reset)
{
	static float time = 0;
	fMainMenu(true);
	DrawRectangle(0, 0, getWidth(), getHeight(), ColorAlpha(BLACK, (1 - time) * 0.7));
	DrawRing((Vector2){.x = getWidth() / 2, .y = getHeight() / 2}, time * getWidth() * 1, time * getWidth() * 0.8, 0, 360, 360, ColorAlpha(WHITE, 1 - time));
	time += fmin(GetFrameTime() / 2, 0.016);
	if (time > 1)
	{
		_pGameplayFunction = &fMainMenu;
	}
}

bool mouseInRect(Rectangle rect)
{
	int x = GetMouseX();
	int y = GetMouseY();
	if (rect.x < x && rect.y < y && rect.x + rect.width > x && rect.y + rect.height > y)
	{
		return true;
	}
	return false;
}

void textBox(Rectangle rect, char *str, bool *selected)
{
	drawButton(rect, str, 0.03);
	if ((mouseInRect(rect) && IsMouseButtonDown(0)) || *selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while (c != 0)
		{
			for (int i = 0; i < 99; i++)
			{
				if (str[i] == '\0')
				{
					str[i] = c;
					str[i + 1] = '\0';
					break;
				}
			}
			c = GetCharPressed();
		}
		if (IsKeyPressed(KEY_BACKSPACE))
		{
			for (int i = 1; i < 99; i++)
			{
				if (str[i] == '\0')
				{
					str[i - 1] = '\0';
					break;
				}
			}
		}
		if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
		{
			*selected = false;
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			char * clipboard = (char*) GetClipboardText();
			int stringLength = strlen(str);
			int cbIndex = 0;
			for (int i = stringLength; i < 98; i++)
			{
				if (clipboard[cbIndex] != '\0')
				{
					str[i] = c;
				}else{
					str[i + 1] = '\0';
					break;
				}
			}
			str[99] = '\0';

			free(clipboard);
		}
	}
	if (*selected)
		DrawRectangle(rect.x + rect.width * 0.2, rect.y + rect.height * 0.75, rect.width * 0.6, rect.height * 0.1, DARKGRAY);

	if (!mouseInRect(rect) && IsMouseButtonDown(0))
		*selected = false;
}

void numberBox(Rectangle rect, int *number, bool *selected)
{
	char str[10];
	snprintf(str, 10, "%i", *number);
	drawButton(rect, str, 0.03);
	if ((mouseInRect(rect) && IsMouseButtonDown(0)) || *selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while (c != 0)
		{
			*number = c - '0';
			c = GetCharPressed();
		}
	}
	if (*selected)
		DrawRectangle(rect.x + rect.width * 0.2, rect.y + rect.height * 0.75, rect.width * 0.6, rect.height * 0.1, DARKGRAY);

	if (!mouseInRect(rect) && IsMouseButtonDown(0))
		*selected = false;
}

void slider(Rectangle rect, bool *selected, int *value, int max, int min)
{

	Color color = WHITE;
	if (*selected)
		color = LIGHTGRAY;

	float sliderMapped = (*value - min) / (float)(max - min);
	DrawRectangle(rect.x, rect.y, rect.width, rect.height, WHITE);
	DrawCircle(rect.x + rect.width * sliderMapped, rect.y + rect.height * 0.5, rect.height, color);


	Rectangle extendedRect = (Rectangle){.x=rect.x-rect.height, .y=rect.y-rect.height/2, .width=rect.width+rect.height*2, .height=rect.height+rect.height};
	if ((mouseInRect(extendedRect) && IsMouseButtonPressed(0)) || (*selected && IsMouseButtonDown(0)))
	{
		*selected = true;
		*value = ((GetMouseX() - rect.x) / rect.width) * (max - min) + min;
	}

	if (!IsMouseButtonDown(0))
		*selected = false;

	if (*value > max)
		*value = max;
	if (*value < min)
		*value = min;
}

bool interactableButton(char *text, float fontScale, float x, float y, float width, float height)
{
	Rectangle button = (Rectangle){.x = x, .y = y, .width = width, .height = height};
	drawButton(button, text, fontScale);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		playAudioEffect(_buttonSe);
		return true;
	}
	return false;
}

bool interactableButtonNoSprite(char *text, float fontScale, float x, float y, float width, float height)
{
	Rectangle button = (Rectangle){.x = x, .y = y, .width = width, .height = height};
	drawButtonNoSprite(button, text, fontScale);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		playAudioEffect(_buttonSe);
		return true;
	}
	return false;
}