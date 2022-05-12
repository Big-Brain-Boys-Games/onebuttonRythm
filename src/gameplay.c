
#include "gameplay.h"
#include "files.h"
#include "drawing.h"
#include "audio.h"

#include "../deps/raylib/src/raylib.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thread.h"
#include <ctype.h>

extern Texture2D _noteTex, _background, _heartTex, _healthBarTex;
extern Color _fade;
extern float _musicPlaying;
extern bool _musicLoops, _playMenuMusic;
extern float _musicHead, _transition;
extern void *_pHitSE, *_pMissHitSE, *_pMissSE, *_pButtonSE, **_pMusic;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size, _buttonSE_Size, _musicFrameCount, *_musicLength;
extern bool _isKeyPressed, _disableLoadingScreen;
extern float _musicSpeed;

extern void *_pFailSE;
extern int _failSE_Size;

extern void *_pFinishSE;
extern int _finishSE_Size;

extern int _loading;

extern Map * _pMaps;
extern char _playerName[100];
float _scrollSpeed = 0.6;
int _noteIndex = 0, _amountNotes = 0;
bool _noBackground = false;
float _health = 50;
int _score = 0, _highScore, _combo = 0, _highestCombo, _highScoreCombo = 0;
float _maxMargin = 0.1;
int _hitPoints = 5;
int _notesMissed = 0;
float _averageAccuracy = 0;
int _missPenalty = 10;
bool _mapRefresh = true;
int _barMeasureCount = 2;

Map *_map;
Settings _settings = (Settings){.volumeGlobal = 50, .volumeMusic = 100, .volumeSoundEffects = 100, .zoom = 7, .offset = 0};

bool showSettings;

// Timestamp of all the notes
Note *_pNotes;
void (*_pNextGameplayFunction)();
void (*_pGameplayFunction)();

char _notfication[100];

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

Modifier *_activeMod[100] = {0}; // we dont even have that many

Modifier _mods[] = {
	(Modifier){.id = 0, .name = "No Fail", .icon = 0, .healthMod = 0, .scoreMod = 0.2, .speedMod = 1, .marginMod = 1},
	(Modifier){.id = 1, .name = "Easy", .icon = 0, .healthMod = 0.5, .scoreMod = 0.5, .speedMod = 1, .marginMod = 2},
	(Modifier){.id = 2, .name = "Hard", .icon = 0, .healthMod = 1.5, .scoreMod = 1.5, .speedMod = 1, .marginMod = 0.5},
	(Modifier){.id = 3, .name = ".5x", .icon = 0, .healthMod = 1, .scoreMod = 0.5, .speedMod = 0.5, .marginMod = 1},
	(Modifier){.id = 4, .name = "2x", .icon = 0, .healthMod = 1, .scoreMod = 1.5, .speedMod = 1.5 /* :P */, .marginMod = 1},
};

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

bool checkFileDropped()
{
	if (IsFileDropped())
	{
		int amount = 0;
		char **files = GetDroppedFiles(&amount);
		for (int i = 0; i < amount; i++)
		{
			if (strcmp(GetFileExtension(files[i]), ".zip") == 0)
			{
				addZipMap(files[i]);
				strcpy(_notfication, "map imported");
			}
		}
		ClearDroppedFiles();
	}
}

void gotoMainMenu(bool mainOrSelect)
{
	stopMusic();
	_playMenuMusic = true;
	randomMusicPoint();
	if (mainOrSelect)
		_pGameplayFunction = &fMainMenu;
	else _pGameplayFunction = &fMapSelect;
	SetWindowTitle("One Button Rhythm");
	resetBackGround();
}

float getMusicHead()
{
	if (_musicPlaying)
		return _musicHead - _settings.offset;
	else
		return _musicHead;
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
	if (mouseInRect(rect) && IsMouseButtonDown(0) || *selected)
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
	}
	if (*selected)
		DrawRectangle(rect.x + rect.width * 0.2, rect.y + rect.height * 0.75, rect.width * 0.6, rect.height * 0.1, DARKGRAY);

	if (!mouseInRect(rect) && IsMouseButtonDown(0))
		*selected = false;
}

void numberBox(Rectangle rect, int *number, bool *selected)
{
	char str[10];
	sprintf(str, "%i", *number);
	drawButton(rect, str, 0.03);
	if (mouseInRect(rect) && IsMouseButtonDown(0) || *selected)
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
	DrawCircle(rect.x + rect.width * sliderMapped + rect.height / 2, rect.y + rect.height * 0.5, rect.height, color);

	if ((mouseInRect(rect) && IsMouseButtonPressed(0)) || (*selected && IsMouseButtonDown(0)))
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
		playAudioEffect(_pButtonSE, _buttonSE_Size);
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
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		return true;
	}
	return false;
}

void removeNote(int index)
{
	_amountNotes--;
	Note *tmp = malloc(sizeof(Note) * _amountNotes);
	memcpy(tmp, _pNotes, sizeof(Note) * _amountNotes);
	for (int i = index; i < _amountNotes; i++)
	{
		tmp[i] = _pNotes[i + 1];
	}
	free(_pNotes);
	_pNotes = tmp;
}

void **_pHitSEp;
void newNote(float time)
{
	printf("new note time %f\n", time);
	int closestIndex = 0;
	_amountNotes++;
	Note *tmp = calloc(_amountNotes, sizeof(Note));
	for (int i = 0; i < _amountNotes - 1; i++)
	{
		tmp[i] = _pNotes[i];
		if (tmp[i].time < time)
		{
			printf("found new closest :%i   value %.2f musicHead %.2f\n", i, tmp[i].time, time);
			closestIndex = i + 1;
		}
	}
	for (int i = closestIndex; i < _amountNotes - 1; i++)
	{
		tmp[i + 1] = _pNotes[i];
	}

	free(_pNotes);
	_pNotes = tmp;
	_pNotes[closestIndex].time = time;
	_pNotes[closestIndex].texture = _noteTex;
	_pHitSEp = &_pHitSE;
	_pNotes[closestIndex].hitSE = &_pHitSEp;
}

void fPause()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();

	dNotes();
	drawVignette();
	drawProgressBar();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	// TODO dynamically change seperation depending on the amount of buttons?
	float middle = GetScreenWidth() / 2;

	if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Continue", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.3, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		if(_pNextGameplayFunction == &fPlaying)
			_pGameplayFunction = &fCountDown;
		else
			_pGameplayFunction = _pNextGameplayFunction;
	}
	if (_pNextGameplayFunction == &fEditor && interactableButton("Save & Exit", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.5, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		saveFile(_amountNotes);
		gotoMainMenu(false);
	}

	if (_pNextGameplayFunction == &fRecording && interactableButton("retry", 0.05, middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.5, GetScreenWidth()*0.3,GetScreenHeight()*0.1))
	{
		_pGameplayFunction = _pNextGameplayFunction;
		_noteIndex = 0;
		_amountNotes = 0;
	}
	
	if(interactableButton("Exit", 0.05, middle - GetScreenWidth()*0.15, _pNextGameplayFunction == &fEditor ? GetScreenHeight() * 0.7 : GetScreenHeight() * 0.5, GetScreenWidth()*0.3,GetScreenHeight()*0.1))
	{
		unloadMap();
		gotoMainMenu(false);
	}
	drawCursor();
}

void fCountDown()
{
	_musicLoops = false;
	_musicPlaying = false;
	static float countDown = 0;
	static bool contin = false;
	if (countDown == 0)
		countDown = GetTime() + 3;
	if (countDown - GetTime() + GetFrameTime() <= 0)
	{
		countDown = 0;
		_pGameplayFunction = _pNextGameplayFunction;
		if (!contin)
		{
			// switching to playing map
			printf("switching to playing map! \n");
			startMusic();

			_health = 50;
			_combo = 0;
			_score = 0;
			_noteIndex = 0;
			_notesMissed = 0;
			_averageAccuracy = 0;
			_musicHead = 0;
			contin = false;
			_scrollSpeed = 4.2 / _map->zoom;
			if (_settings.zoom != 0)
				_scrollSpeed = 4.2 / _settings.zoom;
		}
		return;
	}
	if (getMusicHead() <= 0)
		_musicHead = GetTime() - countDown;
	else
		contin = true;
	ClearBackground(BLACK);
	drawBackground();

	// draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() / 2;

	dNotes();

	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	DrawRectangle(middle - width / 2, 0, width, GetScreenHeight(), (Color){.r = 255, .g = 255, .b = 255, .a = 255 / 2});

	// draw score
	char *tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	drawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.05, WHITE);

	drawCursor();

	float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x = GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale, .y = GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale, WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x = GetScreenWidth() * 0.85, .y = GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale, WHITE);
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	sprintf(tmpString, "%i", (int)(countDown - GetTime() + 1));
	float textSize = measureText(tmpString, GetScreenWidth() * 0.3);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.3, GetScreenWidth() * 0.3, WHITE);
	free(tmpString);
	drawVignette();
}

void fMainMenu()
{
	checkFileDropped();
	_musicLoops = true;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = GetScreenHeight(), .width = GetScreenWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth() / 2;
	drawVignette();
	// draw main menu

	// Rectangle playButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.3, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	//  drawButton(playButton,"play", 0.04);
	//  Rectangle editorButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.45, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	//  drawButton(editorButton,"Editor", 0.04);
	//  Rectangle SettingsButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.60, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	//  drawButton(SettingsButton,"Settings", 0.04);
	//  Rectangle recordingButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.75, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	//  drawButton(recordingButton,"Record", 0.04);

	if (interactableButton("Play", 0.04, middle - GetScreenWidth() * 0.3, GetScreenHeight() * 0.3, GetScreenWidth() * 0.2, GetScreenHeight() * 0.08))
	{
		// switching to playing map
		printf("switching to playing map!\n");
		_pNextGameplayFunction = &fPlaying;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if (interactableButton("Settings", 0.04, middle - GetScreenWidth() * 0.34, GetScreenHeight() * 0.45, GetScreenWidth() * 0.2, GetScreenHeight() * 0.08))
	{
		// Switching to settings
		_pGameplayFunction = &fSettings;
		_transition = 0.1;
		_settings.offset = _settings.offset * 1000;
	}

	// if (interactableButton("Export", 0.04, middle - GetScreenWidth()*0.38,GetScreenHeight() * 0.60,GetScreenWidth()*0.2,GetScreenHeight()*0.08))
	// {
	// 	//Switching to export
	// 	_pNextGameplayFunction = &fExport;
	// 	_pGameplayFunction = &fMapSelect;
	// 	_transition = 0.1;
	// }

	if (interactableButton("New Map", 0.04, middle - GetScreenWidth() * 0.38, GetScreenHeight() * 0.60, GetScreenWidth() * 0.2, GetScreenHeight() * 0.08))
	{
		_pGameplayFunction = &fNewMap;
		_transition = 0.1;
	}

	// gigantic ass title
	char *title = "One Button Rhythm";
	float tSize = GetScreenWidth() * 0.05;
	int size = MeasureText(title, tSize);
	// dropshadow
	drawText(title, middle - size / 2 + GetScreenWidth() * 0.004, GetScreenHeight() * 0.107, tSize, DARKGRAY);
	// real title
	drawText(title, middle - size / 2, GetScreenHeight() * 0.1, tSize, WHITE);

	char str[120];
	sprintf(str, "name: %s", _playerName);
	drawText(str, GetScreenWidth() * 0.4, GetScreenHeight() * 0.8, GetScreenWidth() * 0.04, WHITE);
	
	drawText(_notfication, GetScreenWidth() * 0.6, GetScreenHeight() * 0.7, GetScreenWidth() * 0.02, WHITE);

	drawCursor();
}

void fSettings()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = GetScreenHeight(), .width = GetScreenWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);
	int middle = GetScreenWidth() / 2;

	// gigantic ass settings title
	char *title = "Settings";
	float tSize = GetScreenWidth() * 0.05;
	int size = MeasureText(title, tSize);
	// dropshadow
	drawText(title, middle - size / 2 + GetScreenWidth() * 0.004, GetScreenHeight() * 0.107, tSize, DARKGRAY);
	// real title
	drawText(title, middle - size / 2, GetScreenHeight() * 0.1, tSize, WHITE);

	char zoom[10] = {0};
	if (_settings.zoom != 0)
		sprintf(zoom, "%i", _settings.zoom);
	static bool zoomBoxSelected = false;

	Rectangle zoomBox = (Rectangle){.x = GetScreenWidth() * 0.4, .y = GetScreenHeight() * 0.7, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
	textBox(zoomBox, zoom, &zoomBoxSelected);

	_settings.zoom = atoi(zoom);
	_settings.zoom = fmin(fmax(_settings.zoom, 0), 300);
	tSize = GetScreenWidth() * 0.03;
	size = MeasureText("zoom", tSize);
	drawText("zoom", zoomBox.x + zoomBox.width / 2 - size / 2, zoomBox.y - GetScreenHeight() * 0.05, tSize, WHITE);

	static bool nameBoxSelected = false;
	Rectangle nameBox = (Rectangle){.x = GetScreenWidth() * 0.02, .y = GetScreenHeight() * 0.3, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(nameBox, _playerName, &nameBoxSelected);

	char offset[10] = {0};
	if (_settings.offset != 0)
		sprintf(offset, "%i", (int)_settings.offset);
	static bool offsetBoxSelected = false;
	Rectangle offsetBox = (Rectangle){.x = GetScreenWidth() * 0.4, .y = GetScreenHeight() * 0.85, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
	textBox(offsetBox, offset, &offsetBoxSelected);
	_settings.offset = (float)atoi(offset);
	_settings.offset = fmin(fmax(_settings.offset, 0), 300);
	tSize = GetScreenWidth() * 0.03;
	size = MeasureText("offset", tSize);
	drawText("offset", offsetBox.x + offsetBox.width / 2 - size / 2, offsetBox.y - GetScreenHeight() * 0.05, tSize, WHITE);

	static bool gvBoolSelected = false;
	Rectangle gvSlider = (Rectangle){.x = GetScreenWidth() * 0.35, .y = GetScreenHeight() * 0.3, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.03};
	slider(gvSlider, &gvBoolSelected, &_settings.volumeGlobal, 100, 0);
	tSize = GetScreenWidth() * 0.03;
	size = MeasureText("global volume", tSize);
	drawText("global volume", gvSlider.x + gvSlider.width / 2 - size / 2, gvSlider.y - GetScreenHeight() * 0.05, tSize, WHITE);

	static bool mvBoolSelected = false;
	Rectangle mvSlider = (Rectangle){.x = GetScreenWidth() * 0.35, .y = GetScreenHeight() * 0.45, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.03};
	slider(mvSlider, &mvBoolSelected, &_settings.volumeMusic, 100, 0);
	tSize = GetScreenWidth() * 0.03;
	size = MeasureText("music volume", tSize);
	drawText("music volume", mvSlider.x + mvSlider.width / 2 - size / 2, mvSlider.y - GetScreenHeight() * 0.05, tSize, WHITE);

	static bool aevBoolSelected = false;
	Rectangle aevSlider = (Rectangle){.x = GetScreenWidth() * 0.35, .y = GetScreenHeight() * 0.6, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.03};
	slider(aevSlider, &aevBoolSelected, &_settings.volumeSoundEffects, 100, 0);
	tSize = GetScreenWidth() * 0.03;
	size = MeasureText("sound sffect volume", tSize);
	drawText("sound Effect volume", aevSlider.x + aevSlider.width / 2 - size / 2, aevSlider.y - GetScreenHeight() * 0.05, tSize, WHITE);

	if (interactableButton("Back", 0.03, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
		_settings.offset = _settings.offset * 0.001;
		saveSettings();
		return;
	}

	drawCursor();
}

void fEndScreen()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	int middle = GetScreenWidth() / 2;
	// draw menu

	float textSize = measureText("Finished", GetScreenWidth() * 0.15);
	drawText("Finished", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.05, GetScreenWidth() * 0.15, WHITE);

	char *tmpString = malloc(50);
	sprintf(tmpString, "%s", _highScore < _score ? "New highscore!" : "");
	textSize = measureText(tmpString, GetScreenWidth() * 0.1);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.2, GetScreenWidth() * 0.1, WHITE);

	// draw score
	sprintf(tmpString, "Score: %i Combo %i", _score, _highestCombo);
	textSize = measureText(tmpString, GetScreenWidth() * 0.07);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.4, GetScreenWidth() * 0.07, LIGHTGRAY);

	// draw highscore
	sprintf(tmpString, "Highscore: %i Combo :%i", _highScore, _highScoreCombo);
	textSize = measureText(tmpString, GetScreenWidth() * 0.05);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.5, GetScreenWidth() * 0.05, LIGHTGRAY);

	// draw extra info
	sprintf(tmpString, "Accuracy: %.2f misses :%i", 100 * (1 - _averageAccuracy), _notesMissed);
	textSize = measureText(tmpString, GetScreenWidth() * 0.05);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.6, GetScreenWidth() * 0.05, LIGHTGRAY);
	free(tmpString);

	if (interactableButton("Retry", 0.05, GetScreenWidth() * 0.15, GetScreenHeight() * 0.7, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		// retrying map
		printf("retrying map! \n");

		_pGameplayFunction = &fCountDown;
		_musicHead = 0;
		_transition = 0.1;
	}
	if (interactableButton("Main Menu", 0.05, GetScreenWidth() * 0.15, GetScreenHeight() * 0.85, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fPlaying;
	}
	drawVignette();
	drawCursor();
}

void fEditor()
{
	static bool isPlaying = false;
	_musicPlaying = isPlaying;
	float secondsPerBeat = getMusicDuration() / getBeatsCount() / _barMeasureCount;
	if (isPlaying)
	{
		_musicHead += GetFrameTime() * _musicSpeed;
		if (endOfMusic())
		{
			_musicPlaying = false;
		}
	}
	else
	{
		if (!showSettings)
		{
			// Disable some keybinds during playback
			setMusicFrameCount();
			if (IsKeyPressed(KEY_RIGHT))
			{
				// Snap to closest beat
				_musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
				// Add the offset
				_musicHead = (_musicHead + _map->offset / 1000.0);
				// Add the bps to the music head
				_musicHead += secondsPerBeat;
				// snap it again (it's close enough right?????)
				_musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
			}

			if (IsKeyPressed(KEY_LEFT))
			{
				_musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
				_musicHead = (_musicHead + _map->offset / 1000.0);
				_musicHead -= secondsPerBeat;
				_musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
			}

			if (GetMouseWheelMove() > 0)
				_musicHead += GetFrameTime() * (_scrollSpeed * 2);
			if (GetMouseWheelMove() < 0)
				_musicHead -= GetFrameTime() * (_scrollSpeed * 2);
			if (IsKeyPressed(KEY_UP) || (GetMouseWheelMove() > 0 && IsKeyDown(KEY_LEFT_CONTROL)))
				_scrollSpeed *= 1.2;
			if (IsKeyPressed(KEY_DOWN) || (GetMouseWheelMove() < 0 && IsKeyDown(KEY_LEFT_CONTROL)))
				_scrollSpeed /= 1.2;
			if (_scrollSpeed == 0)
				_scrollSpeed = 0.01;
			if (IsMouseButtonDown(2))
			{
				_musicHead -= GetMouseDelta().x / GetScreenWidth() * _scrollSpeed;
			}

			if (IsMouseButtonPressed(0))
			{
				printf("Note at: %f\n", findClosest(_pNotes, _amountNotes / 2, screenToMusicTime(GetMouseX())));
			}

			if (getMusicHead() < 0)
				_musicHead = 0;

			if (IsKeyPressed(KEY_ESCAPE))
			{
				_pGameplayFunction = &fPause;
				_pNextGameplayFunction = &fEditor;
				return;
			}

			if (getMusicHead() > getMusicDuration())
				_musicHead = getMusicDuration();
			if (_isKeyPressed)
			{
				float closestTime = 55;
				int closestIndex = 0;
				for (int i = 0; i < _amountNotes; i++)
				{
					if (closestTime > fabs(_pNotes[i].time - getMusicHead()))
					{
						closestTime = fabs(_pNotes[i].time - getMusicHead());
						closestIndex = i;
					}
				}
				if (IsKeyPressed(KEY_Z) && closestTime > 0.03f)
				{
					newNote(getMusicHead());
				}

				if (IsKeyPressed(KEY_X) && closestTime < _maxMargin)
				{
					removeNote(closestIndex);
				}

				if (IsKeyPressed(KEY_C) && !isPlaying)
				{
					// todo maybe not 4 subbeats?
					_musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
				}

				if (IsKeyPressed(KEY_E) && _barMeasureCount <= 32)
				{
					_barMeasureCount = _barMeasureCount * 2;
				}

				if (IsKeyPressed(KEY_Q) && _barMeasureCount >= 2)
				{
					_barMeasureCount = _barMeasureCount / 2;
				}
				
			}
		}
	}
	if (IsKeyPressed(KEY_SPACE) && !showSettings)
	{
		isPlaying = !isPlaying;
		_musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
	}
	ClearBackground(BLACK);

	drawBackground();

	// draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() / 2;

	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	dNotes();
	drawMusicGraph(0.4);
	drawVignette();
	drawBars();
	drawProgressBarI(!isPlaying);

	if (showSettings)
	{
		// Darken background
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

		// BPM setting
		char bpm[10] = {0};
		if (_map->bpm != 0)
			sprintf(bpm, "%i", _map->bpm);
		static bool bpmBoxSelected = false;
		Rectangle bpmBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.1, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
		textBox(bpmBox, bpm, &bpmBoxSelected);
		_map->bpm = atoi(bpm);
		_map->bpm = fmin(fmax(_map->bpm, 0), 300);

		// Offset setting
		char offset[10] = {0};
		if (_map->offset != 0)
			sprintf(offset, "%i", _map->offset);
		static bool offsetBoxSelected = false;
		Rectangle offsetBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.18, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
		textBox(offsetBox, offset, &offsetBoxSelected);
		_map->offset = atoi(offset);
		_map->offset = fmin(fmax(_map->offset, 0), 5000);

		// song name setting
		char songName[50] = {0};
		sprintf(songName, "%s", _map->name);
		static bool songNameBoxSelected = false;
		Rectangle songNameBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.26, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
		textBox(songNameBox, songName, &songNameBoxSelected);
		strcpy(_map->name,songName);

		// song creator setting
		char creator[50] = {0};
		sprintf(creator, "%s", _map->artist);
		static bool creatorBoxSelected = false;
		Rectangle creatorBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.34, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
		textBox(creatorBox, creator, &creatorBoxSelected);
		strcpy(_map->artist,creator);

		// Speed slider
		static bool speedSlider = false;
		int speed = _musicSpeed * 4;
		slider((Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.50, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.03}, &speedSlider, &speed, 8, 1);
		_musicSpeed = speed / 4.0;
		if (interactableButton("reset", 0.03, GetScreenWidth() * 0.52, GetScreenHeight() * 0.50, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
		{
			_musicSpeed = 1;
		}

		// Drawing text next to the buttons
		char *text = "BPM:";
		float tSize = GetScreenWidth() * 0.025;
		int size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.12, tSize, WHITE);

		text = "Song Offset:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.20, tSize, WHITE);

		text = "Song name:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.28, tSize, WHITE);

		text = "Artist:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.36, tSize, WHITE);

		text = "Playback speed:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.50, tSize, WHITE);
	}

	if (interactableButton("Song settings", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.05, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		showSettings = !showSettings;
	}
	drawCursor();
}

void fRecording()
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

	if (_isKeyPressed && getMusicHead != 0)
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

bool isAnyKeyDown()
{
	return GetKeyPressed() ||
		   IsMouseButtonPressed(0) ||
		   IsGamepadButtonPressed(0, GetGamepadButtonPressed()) ||
		   IsGamepadButtonPressed(1, GetGamepadButtonPressed()) ||
		   IsGamepadButtonPressed(2, GetGamepadButtonPressed()) ||
		   IsGamepadButtonPressed(3, GetGamepadButtonPressed());
}

#define RippleAmount 10
#define feedback(newFeedback, size)               \
	feedbackSayings[feedbackIndex] = newFeedback; \
	feedbackSize[feedbackIndex] = size;           \
	feedbackIndex++;                              \
	if (feedbackIndex > 4)                        \
		feedbackIndex = 0;
#define addRipple(newRipple)                             \
	rippleEffect[rippleEffectIndex] = 0;                 \
	rippleEffectStrength[rippleEffectIndex] = newRipple; \
	rippleEffectIndex = (rippleEffectIndex + 1) % RippleAmount;
void fPlaying()
{
	static char *feedbackSayings[5];
	static float feedbackSize[5];
	static int feedbackIndex = 0;
	static float rippleEffect[RippleAmount] = {0};
	static float rippleEffectStrength[RippleAmount] = {0};
	static int rippleEffectIndex = 0;
	static float smoothHealth = 50;
	_musicHead += GetFrameTime() * _musicSpeed;
	_musicPlaying = true;
	fixMusicTime();

	_musicSpeed = getSpeedMod();

	if (endOfMusic())
	{
		stopMusic();
		float tmp = 0;
		readScore(_map, &_highScore, &_highScoreCombo, &tmp);
		if (_highScore < _score)
			saveScore();
		_pGameplayFunction = &fEndScreen;
		playAudioEffect(_pFinishSE, _finishSE_Size);
		_transition = 0.1;
		return;
	}
	if (IsKeyPressed(KEY_ESCAPE))
	{
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fPlaying;
	}

	ClearBackground(BLACK);

	drawBackground();

	// draw ripples
	for (int i = 0; i < RippleAmount; i++)
	{
		if (rippleEffectStrength[i] == 0)
			continue;
		rippleEffect[i] += GetFrameTime() * 1200 * rippleEffectStrength[i];
		rippleEffectStrength[i] = fmax(rippleEffectStrength[i] - GetFrameTime() * 5, 0);
		float size = rippleEffect[i];
		DrawRing((Vector2){.x = GetScreenWidth() / 2, .y = GetScreenHeight() * 0.42}, size * GetScreenWidth() * 0.001, size * 0.7 * GetScreenWidth() * 0.001, 0, 360, 50, ColorAlpha(WHITE, rippleEffectStrength[i] * 0.35));
	}

	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() / 2;

	dNotes();

	if (_combo > _highestCombo)
		_highestCombo = _combo;

	// draw feedback (300! 200! miss!)
	for (int i = 0, j = feedbackIndex - 1; i < 5; i++, j--)
	{
		if (j < 0)
			j = 4;
		drawText(feedbackSayings[j], GetScreenWidth() * 0.35, GetScreenHeight() * (0.6 + i * 0.1), GetScreenWidth() * 0.05 * feedbackSize[j], (Color){.r = 255, .g = 255, .b = 255, .a = noLessThanZero(150 - i * 40)});
	}

	if (_noteIndex < _amountNotes && getMusicHead() - _maxMargin > _pNotes[_noteIndex].time)
	{
		// passed note
		_noteIndex++;
		feedback("miss!", 1.3 - _health / 100);
		_health -= _missPenalty * getHealthMod();
		_combo = 0;
		playAudioEffect(_pMissSE, _missSE_Size);
	}

	if (_isKeyPressed && _noteIndex < _amountNotes)
	{
		float closestTime = 55;
		float closestNote = 9999999;
		int closestIndex = 0;
		for (int i = _noteIndex; i <= _noteIndex + 1 && i < _amountNotes; i++)
		{
			if (closestNote > _pNotes[i].time - getMusicHead() - _maxMargin)
			{
				closestNote = _pNotes[i].time - getMusicHead() - _maxMargin;
				closestTime = fabs(_pNotes[i].time - getMusicHead());
				closestIndex = i;
			}
		}
		if (fabs(closestTime) < _maxMargin * getMarginMod())
		{
			while (_noteIndex < closestIndex)
			{
				_noteIndex++;
				feedback("miss!", 1.3 - _health / 100);
				_combo = 0;
				_health -= _missPenalty * getHealthMod();
				_notesMissed++;
			}
			_averageAccuracy = ((_averageAccuracy * (_noteIndex - _notesMissed)) + ((1 / _maxMargin) * closestTime)) / (_noteIndex - _notesMissed + 1);
			// _averageAccuracy = 0.5/_amountNotes;
			int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / _maxMargin * getMarginMod()));
			_health += healthAdded * (1 / (getHealthMod() + 0.1));
			int scoreAdded = noLessThanZero(300 - closestTime * (300 / _maxMargin * getMarginMod())) * getScoreMod();
			if (scoreAdded > 200)
			{
				feedback("300!", 1.2);
				addRipple(1.5);
			}
			else if (scoreAdded > 100)
			{
				feedback("200!", 1);
				addRipple(1);
			}
			else
			{
				feedback("100!", 0.8);
				addRipple(0.6);
			}
			_score += scoreAdded * (1 + _combo / 100);
			_noteIndex++;
			_combo++;
			playAudioEffect(**_pNotes[closestIndex].hitSE, *_pNotes[closestIndex].hitSE_Length);
		}
		else
		{
			printf("missed note\n");
			feedback("miss!", 1.3 - _health / 100);
			_combo = 0;
			_health -= _missPenalty * getHealthMod();
			playAudioEffect(_pMissHitSE, _missHitSE_Size);
			_notesMissed++;
		}
		ClearBackground(BLACK);
	}

	if (_health > 100)
		_health = 100;
	if (_health < 0)
		_health = 0;

	if (_health < 25)
		_fade = ColorAlpha(RED, 1 - _health / 25);
	else
		_fade = ColorAlpha(RED, 0);

	smoothHealth += (_health - smoothHealth) * GetFrameTime() * 3;

	if (feedbackIndex >= 5)
		feedbackIndex = 0;

	float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x = GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale, .y = GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale, WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x = GetScreenWidth() * 0.85, .y = GetScreenHeight() * (0.85 - smoothHealth / 250)}, 0, heartScale, WHITE);

	if (_health <= 0)
	{
		printf("goto fail\n");
		// goto fFail
		stopMusic();
		_pGameplayFunction = &fFail;
		playAudioEffect(_pFailSE, _failSE_Size);
		_transition = 0.1;
	}
	drawVignette();

	// draw score
	char *tmpString = malloc(20);
	sprintf(tmpString, "score: %i", _score);
	drawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.05, WHITE);

	// draw combo
	sprintf(tmpString, "combo: %i", _combo);
	drawText(tmpString, GetScreenWidth() * 0.70, GetScreenHeight() * 0.05, GetScreenWidth() * 0.05, WHITE);

	// draw acc
	//  sprintf(tmpString, "acc: %.5f", (int)(100*_averageAccuracy*(_amountNotes/(_noteIndex+1))));
	//  printf("%.2f   %.2f\n", _averageAccuracy, ((float)_amountNotes/(_noteIndex+1)));
	sprintf(tmpString, "acc: %.2f", 100 * (1 - _averageAccuracy));
	drawText(tmpString, GetScreenWidth() * 0.70, GetScreenHeight() * 0.1, GetScreenWidth() * 0.04, WHITE);
	free(tmpString);
	drawProgressBar();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), _fade);
}

void fFail()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	if (!_noBackground)
	{
		float scale = (float)GetScreenWidth() / (float)_background.width;
		DrawTextureEx(_background, (Vector2){.x = 0, .y = (GetScreenHeight() - _background.height * scale) / 2}, 0, scale, WHITE);
	}
	else
	{
		DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
						 (Rectangle){.x = 0, .y = 0, .height = GetScreenHeight(), .width = GetScreenWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);
	}
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	int middle = GetScreenWidth() / 2;
	// draw menu

	float textSize = measureText("You Failed", GetScreenWidth() * 0.15);
	drawText("You Failed", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.2, GetScreenWidth() * 0.15, WHITE);

	// draw score
	char *tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	textSize = measureText(tmpString, GetScreenWidth() * 0.1);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.5, GetScreenWidth() * 0.1, LIGHTGRAY);
	free(tmpString);

	if (interactableButton("Retry", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.7, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		// retrying map
		printf("retrying map! \n");
		_pGameplayFunction = &fCountDown;
		_musicHead = 0;
		_transition = 0.7;
	}
	if (interactableButton("Exit", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.85, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		// retrying map
		printf("going to main Menu! \n");
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fPlaying;
	}
	drawVignette();
	drawCursor();
}

struct mapInfoLoadingArgs
{
	int *amount;
	int **highScores;
	int **combos;
	float **accuracy;
};

char ** filesCaching = 0;

#ifdef __unix
void mapInfoLoading(struct mapInfoLoadingArgs *args)
#else
DWORD WINAPI *mapInfoLoading(struct mapInfoLoadingArgs *args)
// DWORD WINAPI * decodeAudio(struct decodeAudioArgs * args)
#endif
{
	_loading++;
	int amount;
	static int oldAmount = 0;
	char **files = GetDirectoryFiles("maps/", &amount);
	if(args->highScores != 0)
	{
		*args->highScores = realloc(*args->highScores, amount*sizeof(int));
		*args->combos = realloc(*args->combos, amount*sizeof(int));
		*args->accuracy = realloc(*args->accuracy, amount*sizeof(float));
		char ** tmp = calloc(amount, sizeof(char*));
		for(int i = 0; i < amount && i < oldAmount; i++)
		{
			tmp[i] = filesCaching[i];
		}
		for(int i = oldAmount; i < amount; i++)
			tmp[i] = calloc(100, sizeof(char));
		for(int i = amount; i < oldAmount; i++)
			free(filesCaching[i]);
		free(filesCaching);
		filesCaching = tmp;

		Map * tmp2 = calloc(amount, sizeof(Map));
		for(int i = 0; i < amount && i < oldAmount; i++)
		{
			tmp2[i] = _pMaps[i];
		}
		for(int i = amount; i < oldAmount; i++)
			freeMap(&_pMaps[i]);
		free(_pMaps);
		_pMaps = tmp2;
	}else {
		*args->highScores = malloc(amount*sizeof(int)); 
		*args->combos = malloc(amount*sizeof(int)); 
		*args->accuracy = malloc(amount*sizeof(float)); 
		filesCaching = calloc(amount, sizeof(char*));
		for(int i = 0; i < amount; i++)
			filesCaching[i] = calloc(100, sizeof(char));
		_pMaps = calloc(amount, sizeof(Map));
	}
	oldAmount = amount;
	int mapIndex = 0;
	for (int i = 0; i < amount; i++)
	{
		if (files[i][0] == '.')
			continue;
		// check for cache
		bool cacheHit = false;
		for (int j = 0; j < amount; j++)
		{
			if (!filesCaching[j][0])
				continue;
			if (strcmp(filesCaching[j], files[i]) == 0)
			{
				// cache hit
				printf("cache hit! %s\n", files[i]);
				cacheHit = true;
				if (mapIndex == j)
				{
					break;
				}
				char str[100];
				strcpy(filesCaching[mapIndex], filesCaching[j]);
				_pMaps[mapIndex] = _pMaps[j];
				_pMaps[j] = (Map){0};
				readScore(&_pMaps[mapIndex],
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
		printf("cache miss %s\n", files[i]);
		// cache miss

		freeMap(&_pMaps[mapIndex]);
		_pMaps[mapIndex] = loadMapInfo(files[i]);
		if (_pMaps[mapIndex].name != 0)
		{
			readScore(&_pMaps[mapIndex],
					  &((*args->highScores)[mapIndex]),
					  &((*args->combos)[mapIndex]),
					  &((*args->accuracy)[mapIndex]));
		}

		// caching
		strcpy(filesCaching[mapIndex], files[i]);

		mapIndex++;
	}
	_loading--;
	ClearDirectoryFiles();
	*args->amount = mapIndex;
	free(args);
}

void fMapSelect()
{
	static int amount = 0;
	static int * highScores;
	static int * combos;
	static float * accuracy;
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
					 (Rectangle){.x = 0, .y = 0, .height = GetScreenHeight(), .width = GetScreenWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	if (selectingMods)
	{
		_playMenuMusic = true;
		_musicPlaying = false;
		hoverPeriod = 0;
		drawVignette();
		if (interactableButton("Back", 0.03, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
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
			if (interactableButton(_mods[i].name, 0.03, GetScreenWidth() * (0.2 + x * 0.22), GetScreenHeight() * (0.05 + y * 0.12), GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
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
				drawText(_activeMod[i]->name, GetScreenWidth() * (0.05 + 0.12 * modsSoFar), GetScreenHeight() * 0.9, GetScreenWidth() * 0.03, WHITE);
				modsSoFar++;
			}
		}

		drawCursor();
		return;
	}
	int middle = GetScreenWidth() / 2;
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight()*0.13, BLACK);
	static float menuScroll = 0;
	static float menuScrollSmooth = 0;
	menuScroll += GetMouseWheelMove() * .04;
	menuScrollSmooth += (menuScroll - menuScrollSmooth) * GetFrameTime() * 15;
	if (IsMouseButtonDown(0))
	{ // scroll by dragging
		menuScroll += GetMouseDelta().y / GetScreenHeight();
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

	if (interactableButton("Mods", 0.03, GetScreenWidth() * 0.2, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
	{
		selectingMods = true;
	}

	if (interactableButton("Back", 0.03, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
	}

	textBox((Rectangle){.x=GetScreenWidth() * 0.35, .y=GetScreenHeight() * 0.05, .width=GetScreenWidth() * 0.2, .height=GetScreenHeight() * 0.05}, search, &searchSelected);

	if (hoverMap == -1)
	{
		_musicFrameCount = 1;
		hoverPeriod = 0;
	}
	else
	{
		_disableLoadingScreen = true;
		hoverPeriod += GetFrameTime();
		if (!_musicLength || !*_musicLength)
			_musicFrameCount = 1;
	}
	// draw map button
	Rectangle mapSelectRect = (Rectangle){.x=0, .y=GetScreenHeight()*0.13, .width=GetScreenWidth(), .height=GetScreenHeight()};
	BeginScissorMode(mapSelectRect.x, mapSelectRect.y, mapSelectRect.width, mapSelectRect.height);
	int mapCount = -1;
	for (int i = 0; i < amount; i++)
	{
		// draw maps
		if(search[0] != '\0')
		{
			bool missingLetter = false;
			char str[100];
			sprintf(str, "%s - %s", _pMaps[i].name, _pMaps[i].artist);
			int stringLength = strlen(str);
			for(int j = 0; j < 100 && search[j] != '\0'; j++)
			{
				bool foundOne = false;
				for(int k = 0; k < stringLength; k++)
				{
					if(tolower(search[j]) == tolower(str[k]))
						foundOne = true;
				}
				if(!foundOne)
				{
					missingLetter = true;
					break;
				}
			}
			if(missingLetter)
				continue;
		}

		mapCount++;
		int x = GetScreenWidth() * 0.05;
		if (mapCount % 2 == 1)
			x = GetScreenWidth() * 0.55;
		Rectangle mapButton = (Rectangle){.x = x, .y = menuScrollSmooth * GetScreenHeight() + GetScreenHeight() * ((floor(i / 2) > floor(selectedMap / 2) && selectedMap != -1 ? 0.4 : 0.3) + 0.45 * floor(mapCount / 2)), .width = GetScreenWidth() * 0.4, .height = GetScreenHeight() * 0.4};
		if ((mouseInRect(mapButton) || selectedMap == i) && mouseInRect(mapSelectRect))
		{
			if (hoverPeriod > 1 && hoverPeriod < 2 || !_musicPlaying)
			{
				// play music
				char str[100];
				loadMusic(&_pMaps[i]);
				_playMenuMusic = false;
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
		if (selectedMap == i)
		{
			if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
				selectedMap = -1;

			Rectangle buttons = (Rectangle){.x = mapButton.x, .y = mapButton.y + mapButton.height, .width = mapButton.width, .height = mapButton.height * 0.15 * selectMapTransition};
			if (mouseInRect(buttons) && IsMouseButtonReleased(0) && mouseInRect(mapSelectRect))
			{
				_map = &_pMaps[i];
				loadMap();
				setMusicStart();
				_musicHead = 0;
				printf("selected map!\n");
				_transition = 0.1;
				_disableLoadingScreen = false;
			}

			drawMapThumbnail(mapButton, &_pMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], true);
			if (interactableButtonNoSprite("play", 0.03, mapButton.x, mapButton.y + mapButton.height, mapButton.width * (1 / 3.0) * 1.01, mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect) )
			{
				_pNextGameplayFunction = &fPlaying;
				_pGameplayFunction = &fCountDown;
			}
			if (interactableButtonNoSprite("editor", 0.03, mapButton.x + mapButton.width * (1 / 3.0), mapButton.y + mapButton.height, mapButton.width * (1 / 3.0) * 1.01, mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect) )
			{
				_pNextGameplayFunction = &fEditor;
				_pGameplayFunction = &fEditor;
				startMusic();
				_musicPlaying = false;
			}
			if (interactableButtonNoSprite("export", 0.03, mapButton.x + mapButton.width * (1 / 3.0 * 2), mapButton.y + mapButton.height, mapButton.width * (1 / 3.0), mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect) )
			{
				_pGameplayFunction = &fExport;
			}
			DrawRectangleGradientV(mapButton.x, mapButton.y + mapButton.height, mapButton.width, mapButton.height * 0.05 * selectMapTransition, ColorAlpha(BLACK, 0.3), ColorAlpha(BLACK, 0));
		}
		else
		{
			drawMapThumbnail(mapButton, &_pMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], false);

			if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
			{
				playAudioEffect(_pButtonSE, _buttonSE_Size);
				selectedMap = i;
				selectMapTransition = 0;
				hoverPeriod = 0;
				_musicFrameCount = 1;
			}
		}
	}
	// DrawRectangleGradientV(mapSelectRect.x, mapSelectRect.y, mapSelectRect.width, mapSelectRect.height*0.2, ColorAlpha(BLACK, 1), ColorAlpha(BLACK, 0));
	drawVignette();

	EndScissorMode();

	if (hoverMap != -1 || selectedMap != -1)
	{
		int selMap = selectedMap != -1 ? selectedMap : hoverMap;
		char str[100];
		strcpy(str, _pMaps[selMap].name);
		strcat(str, " - ");
		strcat(str, _pMaps[selMap].artist);
		int textSize = measureText(str, GetScreenWidth() * 0.05);
		drawText(str, GetScreenWidth() * 0.9 - textSize, GetScreenHeight() * 0.92, GetScreenWidth() * 0.05, WHITE);
	}

	drawCursor();
}

void fExport()
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

void fNewMap()
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
		strcpy(newMap.artist, "creator");
		newMap.folder = malloc(100);
		newMap.folder[0] = '\0';
		newMap.bpm = 100;
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = GetScreenHeight(), .width = GetScreenWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth() / 2;

	if (interactableButton("Back", 0.03, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
		return;
	}

	if (interactableButton("Finish", 0.02, GetScreenWidth() * 0.85, GetScreenHeight() * 0.85, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
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
		sprintf(newMap.musicFile, "/song%s", pMusicExt);

		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/image.png");
		file = fopen(str, "wb");
		fwrite(pImage, imageSize, 1, file);
		fclose(file);
		if (newMap.bpm == 0)
			newMap.bpm = 1;
		_pNextGameplayFunction=&fRecording;
		_amountNotes = 0;
		_noteIndex = 0;
		_pGameplayFunction=&fCountDown;
		_transition = 0.1;
		newMap.folder = malloc(100);
		strcpy(newMap.folder, newMap.name);
		_map = &newMap;
		saveFile(0);
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
	Rectangle nameBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.5, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
	textBox(nameBox, newMap.name, &nameBoxSelected);
	drawText("name", middle, GetScreenHeight() * 0.45, GetScreenHeight() * 0.05, WHITE);

	static bool creatorBoxSelected = false;
	Rectangle creatorBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.625, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
	textBox(creatorBox, newMap.artist, &creatorBoxSelected);
	drawText("creator", middle, GetScreenHeight() * 0.575, GetScreenHeight() * 0.05, WHITE);

	char str[100] = {'\0'};
	if (newMap.bpm != 0)
		sprintf(str, "%i", newMap.bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.875, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
	textBox(bpmBox, str, &bpmBoxSelected);
	newMap.bpm = fmin(fmax(atoi(str), 0), 500);
	drawText("bpm", middle, GetScreenHeight() * 0.825, GetScreenHeight() * 0.05, WHITE);

	static bool difficultyBoxSelected = false;
	Rectangle difficultyBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.75, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
	numberBox(difficultyBox, &newMap.difficulty, &difficultyBoxSelected);
	if (newMap.difficulty < 0)
		newMap.difficulty = 0;
	if (newMap.difficulty > 9)
		newMap.difficulty = 0;
	drawText("difficulty", middle, GetScreenHeight() * 0.70, GetScreenHeight() * 0.05, WHITE);

	int textSize = measureText("Drop in .png, .wav or .mp3", GetScreenWidth() * 0.04);
	drawText("Drop in .png, .wav or .mp3", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight() * 0.2, GetScreenWidth() * 0.04, WHITE);

	textSize = measureText("missing music file", GetScreenWidth() * 0.03);
	if (pMusic == 0)
		drawText("missing music file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight() * 0.6, GetScreenWidth() * 0.03, WHITE);
	else
		drawText("got music file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight() * 0.6, GetScreenWidth() * 0.03, WHITE);

	textSize = measureText("missing image file", GetScreenWidth() * 0.03);
	if (pImage == 0)
		drawText("missing image file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight() * 0.7, GetScreenWidth() * 0.03, WHITE);
	else
		drawText("got image file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight() * 0.7, GetScreenWidth() * 0.03, WHITE);

	drawCursor();

	// file dropping
	if (IsFileDropped() || IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
	{
		printf("yoo new file dropped boys\n");
		int amount = 0;
		char **files;
		bool keyOrDrop = true;
		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			// copy paste
			const char *str = GetClipboardText();

			int file = 0;
			char part[100] = {0};
			int partIndex = 0;
			// todo no hardcode, bad hardcode!
			files = malloc(100);
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
			files = GetDroppedFiles(&amount);
		for (int i = 0; i < amount; i++)
		{
			const char *ext = GetFileExtension(files[i]);
			printf("%s\n", ext);
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
			ClearDroppedFiles();
	}
}

void fIntro()
{
	static float time = 0;
	fMainMenu();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, (1 - time) * 0.7));
	DrawRing((Vector2){.x = GetScreenWidth() / 2, .y = GetScreenHeight() / 2}, time * GetScreenWidth() * 1, time * GetScreenWidth() * 0.8, 0, 360, 360, ColorAlpha(WHITE, 1 - time));
	time += fmin(GetFrameTime() / 2, 0.016);
	if (time > 1)
	{
		_pGameplayFunction = &fMainMenu;
	}
}