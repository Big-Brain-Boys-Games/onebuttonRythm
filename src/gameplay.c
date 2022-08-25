
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
#include <time.h>

extern Texture2D _noteTex, _background, _heartTex, _healthBarTex;
extern Color _fade;
extern float _musicPlaying;
extern bool _musicLoops, _playMenuMusic;
extern double _musicHead;
extern float _transition;
extern void *_pHitSE, *_pMissHitSE, *_pMissSE, *_pButtonSE, **_pMusic;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size, _buttonSE_Size, _musicFrameCount, *_musicLength;
extern bool _isKeyPressed, _disableLoadingScreen;
extern float _musicSpeed, *_musicPreviewOffset;

extern void *_pFailSE;
extern int _failSE_Size;

extern void *_pFinishSE;
extern int _finishSE_Size;

extern int _loading;

extern Map *_pMaps;
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
int _missPenalty = 5;
bool _mapRefresh = true;
int _barMeasureCount = 2;

Note **_selectedNotes = 0;
int _amountSelectedNotes = 0;

Map *_map;
Settings _settings = (Settings){.volumeGlobal = 50, .volumeMusic = 100, .volumeSoundEffects = 100, .zoom = 7, .offset = 0};

bool _showSettings = false;
bool _showNoteSettings = false;
bool _showAnimation = false;


// Timestamp of all the notes
Note ** _papNotes;
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

float getMusicHead()
{
	if (_musicPlaying)
		return _musicHead;
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

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			char * clipboard = GetClipboardText();
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
	for(int i = 0; i < _amountSelectedNotes; i++)
		if(_selectedNotes[i] == _papNotes[index])
		{
			removeSelectedNote(i);
			break;
		}
	_amountNotes--;
	free(_papNotes[index]);
	for (int i = index; i < _amountNotes; i++)
	{
		_papNotes[i] = _papNotes[i + 1];
	}
	_papNotes = realloc(_papNotes, (_amountNotes+1) * sizeof(Note*));
}

int newNote(float time)
{
	printf("amountnotes %i\n", _amountNotes);
	int closestIndex = 0;
	_amountNotes++;
	_papNotes = realloc(_papNotes, (_amountNotes+1)* sizeof(Note*));
	for (int i = 0; i < _amountNotes - 1; i++)
	{
		_papNotes[i] = _papNotes[i];
		if (_papNotes[i]->time < time)
		{
			closestIndex = i + 1;
		}else break;
	}
	for (int i = _amountNotes-2; i >= closestIndex; i--)
	{
		_papNotes[i + 1] = _papNotes[i];
	}
	_papNotes[closestIndex] = calloc(1, sizeof(Note));
	_papNotes[closestIndex]->time = time;
	_papNotes[closestIndex]->health = 1;
	_papNotes[closestIndex]->hit = 0;
	return closestIndex;
}

void addSelectNote(int note)
{
	if(note == -1)
		return;
	printf("Adding note: %i  %i\n", note, _amountSelectedNotes);
	for (int i = 0; i < _amountSelectedNotes; i++)
	{
		if (_selectedNotes[i] == _papNotes[note])
		{
			printf("Note in array, removing: %i at index: %i\n", note, i);
			removeSelectedNote(i);
			return;
		}
	}

	_amountSelectedNotes++;
	if (_selectedNotes == 0)
		_selectedNotes = malloc(sizeof(Note*) * _amountSelectedNotes);
	else
		_selectedNotes = realloc(_selectedNotes, sizeof(Note*) * _amountSelectedNotes);
	_selectedNotes[_amountSelectedNotes - 1] = _papNotes[note];
}

void removeSelectedNote(int index)
{
	printf("Removing note: %i  amount %i\n", index, _amountSelectedNotes);

	for (int i = index; i < _amountSelectedNotes-1; i++)
	{
		_selectedNotes[i] = _selectedNotes[i + 1];
		printf("%i <- %i\n", i, i+1);
	}
	_amountSelectedNotes--;
	_selectedNotes = realloc(_selectedNotes, sizeof(Note *) * _amountSelectedNotes);
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
		if (_pNextGameplayFunction == &fPlaying)
			_pGameplayFunction = &fCountDown;
		else
			_pGameplayFunction = _pNextGameplayFunction;
	}
	if (_pNextGameplayFunction == &fEditor && interactableButton("Save & Exit", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.5, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		saveFile(_amountNotes);
		gotoMainMenu(false);
	}

	if (_pNextGameplayFunction == &fPlaying && interactableButton("retry", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.5, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		_pGameplayFunction = fCountDown;
		_noteIndex = 0;
		_amountNotes = 0;
		_musicHead = 0;
	}

	if (_pNextGameplayFunction == &fRecording && interactableButton("retry", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.5, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		_pGameplayFunction = _pNextGameplayFunction;
		_noteIndex = 0;
		_amountNotes = 0;
		_musicHead = 0;
	}

	if (interactableButton("Exit", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.7, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
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
			countDown = 0;
			_scrollSpeed = 4.2 / _map->zoom;
			if (_settings.zoom != 0)
				_scrollSpeed = 4.2 / _settings.zoom;

			//set all note.hits to false
			for(int i = 0; i < _amountNotes; i++)
				_papNotes[i]->hit = false;
		}
		contin = false;
		return;
	}
	if (_musicHead <= 0)
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
	snprintf(tmpString, 9, "%i", _score);
	drawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.05, WHITE);

	drawCursor();

	float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x = GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale, .y = GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale, WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x = GetScreenWidth() * 0.85, .y = GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale, WHITE);
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	snprintf(tmpString, 9, "%i", (int)(countDown - GetTime() + 1));
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

	// if (interactableButton("Play", 0.04, middle - GetScreenWidth() * 0.3, GetScreenHeight() * 0.3, GetScreenWidth() * 0.2, GetScreenHeight() * 0.08))
	// {
	// 	// switching to playing map
	// 	printf("switching to playing map!\n");
	// 	_pNextGameplayFunction = &fPlaying;
	// 	_pGameplayFunction = &fMapSelect;
	// 	_transition = 0.1;
	// }
	float fontScale = 0.075;
	//play button
	Rectangle button = (Rectangle){.x = GetScreenWidth() * 0.05, .y = GetScreenHeight() * 0.25, .width = GetScreenWidth() * 0.32, .height = GetScreenWidth() * 0.32};
	Color color = WHITE;
	static float growTimer = 0;
	if(mouseInRect(button))
	{
		growTimer += GetFrameTime() * 12;
		color = LIGHTGRAY;
	}else
		growTimer -= GetFrameTime() * 12;

	growTimer = fmax(fmin(growTimer, 1), 0);
	
	button.width += GetScreenWidth()*0.1*growTimer;
	button.height += GetScreenWidth()*0.1*growTimer;
	button.x -= GetScreenWidth()*0.05*growTimer;
	button.y -= GetScreenWidth()*0.05*growTimer;

	DrawTexturePro(_noteTex, (Rectangle){.x=0, .y=0, .width=_noteTex.width, .height=_noteTex.height}, (Rectangle){.x=button.x, .y=button.y, .width=button.width, .height=button.width}, (Vector2) {.x=0, .y=0}, 0, WHITE);

	// DrawTexturePro(_noteTex, (Rectangle){.x=0, .y=0, .width=_noteTex.width, .height=_noteTex.height}, (Rectangle){.x=0, .y=0, .width=button.width, .height=button.height}, (Vector2){.x=button.x, .y=button.y}, 0, WHITE);
	// drawBox(button, color);
	fontScale *= 1.3;
	// DrawRectangle(rect.x, rect.y, rect.width, rect.height, ColorAlpha(color, 0.5));
	int screenSize = GetScreenWidth() > GetScreenHeight() ? GetScreenHeight() : GetScreenWidth();
	int textSize = measureText("Play", screenSize * fontScale);
	drawText("Play", button.x + button.width / 2 - textSize / 2, button.y + button.height*0.5-GetScreenHeight()*0.045, screenSize * fontScale, DARKGRAY);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		printf("switching to playing map!\n");
		_pNextGameplayFunction = &fPlaying;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if (interactableButton("Settings", 0.035, middle - GetScreenWidth() * (0.12-growTimer*0.03), GetScreenHeight() * 0.55, GetScreenWidth() * 0.2, GetScreenHeight() * 0.065))
	{
		// Switching to settings
		_pGameplayFunction = &fSettings;
		_transition = 0.1;
	}

	if (interactableButton("New Map", 0.035, middle - GetScreenWidth() * (0.03 - growTimer*0.03), GetScreenHeight() * 0.65, GetScreenWidth() * 0.2, GetScreenHeight() * 0.065))
	{
		_pGameplayFunction = &fNewMap;
		_transition = 0.1;
	}

	// gigantic ass title
	char *title = "One Button";
	float tSize = GetScreenWidth() * 0.07;
	int size = MeasureText(title, tSize);
	Vector2 titlePos = (Vector2){.x=GetScreenWidth()*0.5, .y=GetScreenHeight()*0.2};
	// dropshadow
	drawText("One Button", titlePos.x + GetScreenWidth() * 0.004, titlePos.y + GetScreenHeight()* 0.007, tSize, ColorAlpha(BLACK, 0.4));
	// real title
	drawText("One Button", titlePos.x, titlePos.y, tSize, WHITE);

	titlePos.x += GetScreenWidth()*0.07;
	titlePos.y += GetScreenHeight()*0.1;
	// dropshadow
	drawText("Rhythm", titlePos.x + GetScreenWidth() * 0.004, titlePos.y + GetScreenHeight()* 0.007, tSize, ColorAlpha(BLACK, 0.4));
	// real title
	drawText("Rhythm", titlePos.x, titlePos.y, tSize, WHITE);

	char str[120];
	snprintf(str, 120, "name: %s", _playerName);
	drawText(str, GetScreenWidth() * 0.55, GetScreenHeight() * 0.92, GetScreenWidth() * 0.04, WHITE);

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

	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight()*0.13, ColorAlpha(BLACK ,0.4));
	// gigantic ass settings title
	char *title = "Settings";
	float tSize = GetScreenWidth() * 0.05;
	int size = MeasureText(title, tSize);
	// dropshadow
	drawText(title, middle - size / 2 + GetScreenWidth() * 0.004, GetScreenHeight() * 0.03, tSize, DARKGRAY);
	// real title
	drawText(title, middle - size / 2, GetScreenHeight() * 0.02, tSize, WHITE);

	static float menuScroll = 0;
	static float menuScrollSmooth = 0;
	menuScroll += GetMouseWheelMove() * .04;
	menuScrollSmooth += (menuScroll - menuScrollSmooth) * GetFrameTime() * 15;
	if (IsMouseButtonDown(0))
	{ // scroll by dragging
		menuScroll += GetMouseDelta().y / GetScreenHeight();
	}

	menuScroll = (int)fmin(fmax(menuScroll, 0), 1);

	Rectangle settingsRect = (Rectangle){.x=0, .y=GetScreenHeight()*0.13, .width=GetScreenWidth(), .height=GetScreenHeight()};
	BeginScissorMode(settingsRect.x, settingsRect.y, settingsRect.width, settingsRect.height);

		char zoom[10] = {0};
		if (_settings.zoom != 0)
			snprintf(zoom, 10, "%i", _settings.zoom);
		static bool zoomBoxSelected = false;

		Rectangle zoomBox = (Rectangle){.x = GetScreenWidth() * 0.1, .y = GetScreenHeight() * (0.7+menuScrollSmooth), .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
		textBox(zoomBox, zoom, &zoomBoxSelected);

		if(!mouseInRect(settingsRect))
			zoomBoxSelected = false;

		_settings.zoom = atoi(zoom);
		_settings.zoom = fmin(fmax(_settings.zoom, 0), 300);
		tSize = GetScreenWidth() * 0.03;
		size = MeasureText("zoom", tSize);
		drawText("zoom", zoomBox.x + zoomBox.width / 2 - size / 2, zoomBox.y - GetScreenHeight() * 0.05, tSize, WHITE);

		static bool nameBoxSelected = false;
		Rectangle nameBox = (Rectangle){.x = GetScreenWidth() * 0.52, .y = GetScreenHeight() * (0.3+menuScrollSmooth), .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(nameBox, _playerName, &nameBoxSelected);

		char offset[10] = {0};
		if (_settings.offset != 0)
			snprintf(offset, 10, "%i", (int)(_settings.offset*1000));
		static bool offsetBoxSelected = false;
		Rectangle offsetBox = (Rectangle){.x = GetScreenWidth() * 0.1, .y = GetScreenHeight() * (0.85+menuScrollSmooth), .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.07};
		textBox(offsetBox, offset, &offsetBoxSelected);
		_settings.offset = (float)atoi(offset);
		_settings.offset = fmin(fmax(_settings.offset, -300), 300);
		_settings.offset *= 0.001;

		if(offsetBoxSelected && IsKeyPressed(KEY_MINUS))
		{
			_settings.offset *= -1;
		}
		

		static bool offsetSliderSelected = false;
		Rectangle offsetSlider = (Rectangle){.x = GetScreenWidth() * 0.1, .y = GetScreenHeight() * (0.9+menuScrollSmooth), .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.03};
		int tempOffset = _settings.offset * 1000;
		slider(offsetSlider, &offsetSliderSelected, &tempOffset, 300, -300);
		_settings.offset = tempOffset * 0.001;

		tSize = GetScreenWidth() * 0.03;
		size = MeasureText("offset", tSize);
		drawText("offset", offsetBox.x + offsetBox.width / 2 - size / 2, offsetBox.y - GetScreenHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			offsetBoxSelected = false;

		static bool gvBoolSelected = false;
		Rectangle gvSlider = (Rectangle){.x = GetScreenWidth() * 0.05, .y = GetScreenHeight() * (0.3+menuScrollSmooth), .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.03};
		slider(gvSlider, &gvBoolSelected, &_settings.volumeGlobal, 100, 0);
		tSize = GetScreenWidth() * 0.03;
		size = MeasureText("global volume", tSize);
		drawText("global volume", gvSlider.x + gvSlider.width / 2 - size / 2, gvSlider.y - GetScreenHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			gvBoolSelected = false;

		static bool mvBoolSelected = false;
		Rectangle mvSlider = (Rectangle){.x = GetScreenWidth() * 0.05, .y = GetScreenHeight() * (0.45+menuScrollSmooth), .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.03};
		slider(mvSlider, &mvBoolSelected, &_settings.volumeMusic, 100, 0);
		tSize = GetScreenWidth() * 0.03;
		size = MeasureText("music volume", tSize);
		drawText("music volume", mvSlider.x + mvSlider.width / 2 - size / 2, mvSlider.y - GetScreenHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			mvBoolSelected = false;


		static bool aevBoolSelected = false;
		Rectangle aevSlider = (Rectangle){.x = GetScreenWidth() * 0.05, .y = GetScreenHeight() * (0.6+menuScrollSmooth), .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.03};
		slider(aevSlider, &aevBoolSelected, &_settings.volumeSoundEffects, 100, 0);
		tSize = GetScreenWidth() * 0.03;
		size = MeasureText("sound sffect volume", tSize);
		drawText("sound Effect volume", aevSlider.x + aevSlider.width / 2 - size / 2, aevSlider.y - GetScreenHeight() * 0.05, tSize, WHITE);

		if(!mouseInRect(settingsRect))
			aevBoolSelected = false;

		drawVignette();
	EndScissorMode();

	if (interactableButton("Back", 0.03, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
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

	char *tmpString = malloc(70);
	snprintf(tmpString, 70, "%s", _highScore < _score ? "New highscore!" : "");
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.55, GetScreenHeight() * 0.3, GetScreenWidth() * 0.05, WHITE);

	// draw highscore
	snprintf(tmpString, 70,"Highscore\nCombo");
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.55, GetScreenHeight() * 0.4, GetScreenWidth() * 0.05, LIGHTGRAY);

	// draw highscore values
	snprintf(tmpString, 70,"%i\n%i", _highScore, _highScoreCombo);
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.83, GetScreenHeight() * 0.4, GetScreenWidth() * 0.05, LIGHTGRAY);



	// draw score
	snprintf(tmpString, 70,"Score\nCombo");
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.06, GetScreenHeight() * 0.3, GetScreenWidth() * 0.05, LIGHTGRAY);

	// draw score Values
	snprintf(tmpString, 70,"%i\n%i", _score, _highestCombo);
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.4, GetScreenHeight() * 0.3, GetScreenWidth() * 0.05, LIGHTGRAY);



	// draw extra info
	snprintf(tmpString, 70,"Accuracy\nMisses");
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.06, GetScreenHeight() * 0.5, GetScreenWidth() * 0.05, LIGHTGRAY);

	snprintf(tmpString, 70,"Acc: %.2fms", _averageAccuracy);
	drawHint((Rectangle){.x=GetScreenWidth() * 0.06, .y=GetScreenHeight() * 0.5, .width=textSize, .height=GetScreenWidth() * 0.2}, tmpString);

	// draw extra info Values
	snprintf(tmpString, 70,"%.1f\n%i", 100 * (1 - _averageAccuracy), _notesMissed);
	textSize = measureText(tmpString, GetScreenWidth() * 0.045);
	drawText(tmpString, GetScreenWidth() * 0.4, GetScreenHeight() * 0.5, GetScreenWidth() * 0.05, LIGHTGRAY);


	free(tmpString);

	drawRank(GetScreenWidth()*0.7, GetScreenHeight()*0.65, GetScreenWidth()*0.2, GetScreenWidth()*0.2, _averageAccuracy);

	if (interactableButton("Retry", 0.05, GetScreenWidth() * 0.15, GetScreenHeight() * 0.72, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
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

#define UNDOBUFFER 5
Note * _paUndoBuffer[UNDOBUFFER] = {0};
int _undoBufferSize[UNDOBUFFER] = {0};
int _undoBufferIndex = 0;

#define freeArray(arr) \
	if(arr)\
		free(arr);\
	arr = 0;

void undo()
{
	printf("undo\n");
	free(_selectedNotes);
	_selectedNotes = 0;
	_amountSelectedNotes = 0;
	_undoBufferIndex--;
	for(int i = 0; i < _amountNotes; i++) //free _papNotes
	{
		freeArray(_papNotes[i]->anim);
		if(_papNotes[i]->hitSE_File)
		{
			char tmp [100];
			snprintf(tmp, 100, "maps/%s/%s", _map->folder, _papNotes[i]->hitSE_File);
			removeCustomSound(tmp);
		}
		if(_papNotes[i]->texture_File)
		{
			char tmp [100];
			snprintf(tmp, 100, "maps/%s/%s", _map->folder, _papNotes[i]->texture_File);
			removeCustomTexture(tmp);
		}
		freeArray(_papNotes[i]->hitSE_File);
		freeArray(_papNotes[i]->texture_File);
		free(_papNotes[i]);
	}
	free(_papNotes);
	_amountNotes = _undoBufferSize[_undoBufferIndex];
	_papNotes = malloc(_amountNotes * sizeof(Note*));
	for(int i = 0; i < _amountNotes; i++)
	{
		_papNotes[i] = malloc(sizeof(Note));
		*_papNotes[i] = _paUndoBuffer[_undoBufferIndex][i];
		_papNotes[i]->anim = 0;
		_papNotes[i]->custSound = 0;
		_papNotes[i]->custTex = 0;
		if(_papNotes[i]->animSize != 0)
		{
			_papNotes[i]->anim = malloc(_papNotes[i]->animSize*sizeof(Frame));
			for(int j = 0; j < _papNotes[i]->animSize; j++) //copy over animation
			{
				_papNotes[i]->anim[j] = _paUndoBuffer[_undoBufferIndex][i].anim[j];
			}
		}
		if(_papNotes[i]->hitSE_File != 0)
		{
			_papNotes[i]->hitSE_File = malloc(strlen(_paUndoBuffer[_undoBufferIndex][i].hitSE_File));
			strcpy(_papNotes[i]->hitSE_File, _paUndoBuffer[_undoBufferIndex][i].hitSE_File);
			char tmp [100];
			snprintf(tmp, 100, "maps/%s/%s", _map->folder, _papNotes[i]->hitSE_File);
			_papNotes[i]->custSound = addCustomSound(tmp);
		}
		if(_papNotes[i]->texture_File != 0)
		{
			_papNotes[i]->texture_File = malloc(strlen(_paUndoBuffer[_undoBufferIndex][i].texture_File));
			strcpy(_papNotes[i]->texture_File, _paUndoBuffer[_undoBufferIndex][i].texture_File);
			char tmp [100];
			snprintf(tmp, 100, "maps/%s/%s", _map->folder, _papNotes[i]->texture_File);
			_papNotes[i]->custTex = addCustomTexture(tmp);
		}
	}
	if(_undoBufferIndex < 0)
		_undoBufferIndex = UNDOBUFFER-1;
}

void doAction()
{
	printf("adding to undo buffer\n");
	_undoBufferIndex++;
	if(_undoBufferIndex >= UNDOBUFFER)
		_undoBufferIndex = 0;

	//free old buffer
	for(int i = 0; i < _undoBufferSize[_undoBufferIndex]; i++)
	{
		freeArray(_paUndoBuffer[_undoBufferIndex][i].anim);
		if(_paUndoBuffer[_undoBufferIndex][i].hitSE_File) removeCustomSound(_paUndoBuffer[_undoBufferIndex][i].hitSE_File);
		if(_paUndoBuffer[_undoBufferIndex][i].texture_File) removeCustomTexture(_paUndoBuffer[_undoBufferIndex][i].texture_File);
		freeArray(_paUndoBuffer[_undoBufferIndex][i].hitSE_File);
		freeArray(_paUndoBuffer[_undoBufferIndex][i].texture_File);
	}
	_paUndoBuffer[_undoBufferIndex] = realloc(_paUndoBuffer[_undoBufferIndex], _amountNotes * sizeof(Note));
	_undoBufferSize[_undoBufferIndex] = _amountNotes;
	//copy over new stuff
	for(int i = 0; i < _amountNotes; i++)
	{
		
		_paUndoBuffer[_undoBufferIndex][i] = *_papNotes[i];
		if(_paUndoBuffer[_undoBufferIndex][i].anim != 0)
		{
			_paUndoBuffer[_undoBufferIndex][i].anim = malloc(_paUndoBuffer[_undoBufferIndex][i].animSize*sizeof(Frame));
			for(int j = 0; j < _paUndoBuffer[_undoBufferIndex][i].animSize; j++) //copy over animation
			{
				_paUndoBuffer[_undoBufferIndex][i].anim[j] = _papNotes[i]->anim[j];
			}
		}
		if(_paUndoBuffer[_undoBufferIndex][i].hitSE_File != 0)
		{
			_paUndoBuffer[_undoBufferIndex][i].hitSE_File = malloc(100);
			strcpy(_paUndoBuffer[_undoBufferIndex][i].hitSE_File, _papNotes[i]->hitSE_File);
			char tmp [100];
			snprintf(tmp, 100, "maps/%s/%s", _map->folder, _papNotes[i]->hitSE_File);
			_paUndoBuffer[_undoBufferIndex][i].custSound = addCustomSound(tmp);
		}
		if(_paUndoBuffer[_undoBufferIndex][i].texture_File != 0)
		{
			_paUndoBuffer[_undoBufferIndex][i].texture_File = malloc(100);
			strcpy(_paUndoBuffer[_undoBufferIndex][i].texture_File, _papNotes[i]->texture_File);
			char tmp [100];
			snprintf(tmp, 100, "maps/%s/%s", _map->folder, _papNotes[i]->texture_File);
			_paUndoBuffer[_undoBufferIndex][i].custTex = addCustomTexture(tmp);
		}
	}
}


void editorSettings()
{
	if (_showSettings)
	{
		// Darken background
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});
		// BPM setting
		char bpm[10] = {0};
		if (_map->bpm != 0)
			snprintf(bpm, 10, "%i", _map->bpm);
		static bool bpmBoxSelected = false;
		Rectangle bpmBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.1, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(bpmBox, bpm, &bpmBoxSelected);
		_map->bpm = atoi(bpm);
		_map->bpm = fmin(fmax(_map->bpm, 0), 300);

		// Offset setting
		char offset[10] = {0};
		if (_map->offset != 0)
			snprintf(offset, 10, "%i", _map->offset);
		static bool offsetBoxSelected = false;
		Rectangle offsetBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.18, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(offsetBox, offset, &offsetBoxSelected);
		_map->offset = atoi(offset);
		_map->offset = fmin(fmax(_map->offset, 0), 5000);

		// song name setting
		char songName[50] = {0};
		snprintf(songName, 50, "%s", _map->name);
		static bool songNameBoxSelected = false;
		Rectangle songNameBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.26, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(songNameBox, songName, &songNameBoxSelected);
		strcpy(_map->name, songName);

		// song creator setting
		char creator[50] = {0};
		snprintf(creator, 50, "%s", _map->artist);
		static bool creatorBoxSelected = false;
		Rectangle creatorBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.34, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(creatorBox, creator, &creatorBoxSelected);
		strcpy(_map->artist, creator);

		// preview offset setting
		char musicPreviewOffset[10] = {0};
		musicPreviewOffset[0] = '\0';
		if (_map->musicPreviewOffset != 0)
			snprintf(musicPreviewOffset, 10, "%i", (int)(_map->musicPreviewOffset * 1000));
		static bool musicPreviewOffsetBoxSelected = false;
		Rectangle musicPreviewOffsetBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.42, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(musicPreviewOffsetBox, musicPreviewOffset, &musicPreviewOffsetBoxSelected);
		_map->musicPreviewOffset = atoi(musicPreviewOffset) / 1000.0;
		_map->musicPreviewOffset = fmin(fmax(_map->musicPreviewOffset, 0), *_musicLength);

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

		text = "Preview offset:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() *  0.42, tSize, WHITE);


		// text = "Playback speed:";
		// tSize = GetScreenWidth() * 0.025;
		// size = measureText(text, tSize);
		// drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.50, tSize, WHITE);
	}
}

void editorNoteSettings()
{
	// Darken background
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.5));
	if(_showAnimation)
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),  ColorAlpha(BLACK, 0.3));

	if (interactableButton("Animation", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.5, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		//Run animation tab
		_showAnimation = !_showAnimation;
		doAction();
		return;
	}

	if (interactableButton("back", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.15, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		_showNoteSettings = !_showNoteSettings;
		//apply changes to first note to all selected notes
		printf("applying to all selected notes\n");
		Note * firstNote = _selectedNotes[0];
		printf("new size %i\n", firstNote->animSize);
		for(int i = 1; i < _amountSelectedNotes; i++)
		{
			if(firstNote->anim != 0 && firstNote->animSize > 0)
			{
				if(_selectedNotes[i]->anim == 0)
					_selectedNotes[i]->anim = malloc(sizeof(Frame)*firstNote->animSize);
				else
					_selectedNotes[i]->anim = realloc(_selectedNotes[i]->anim, sizeof(Frame)*firstNote->animSize);
				
				_selectedNotes[i]->animSize = firstNote->animSize;

				for(int key = 0; key < firstNote->animSize; key++)
				{
					_selectedNotes[i]->anim[key] = firstNote->anim[key];
				}
			}
		}
		return;
	}

	if(!_showAnimation)
	{
		char sprite[100] = {0};
		sprite[0] = '\0';
		if (_selectedNotes[0]->texture_File != 0)
			snprintf(sprite, 100, "%s", _selectedNotes[0]->texture_File);
		static bool spriteBoxSelected = false;
		Rectangle spriteBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.1, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(spriteBox, sprite, &spriteBoxSelected);
		if (strlen(sprite) != 0)
		{
			if (_selectedNotes[0]->texture_File == 0)
				_selectedNotes[0]->texture_File = malloc(sizeof(char) * 100);
			strcpy(_selectedNotes[0]->texture_File, sprite);
		}else if(_selectedNotes[0]->texture_File != 0)
		{
			free(_selectedNotes[0]->texture_File);
			_selectedNotes[0]->texture_File = 0;
		}

		char *text = "sprite file:";
		float tSize = GetScreenWidth() * 0.025;
		int size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.1, tSize, WHITE);

		

		// health setting
		char health[10] = {0};
		bool theSame = false;
		for (int i = 0; i < _amountSelectedNotes; i++)
		{
			if (_selectedNotes[0]->health != _selectedNotes[i]->health) {
				theSame = false;
				break;
			}
			else
				theSame = true;
		}
		if (theSame)
		{
			snprintf(health, 10, "%i", (int)(_selectedNotes[0]->health));
		}
		else snprintf(health, 10, "-", "%c");
		
		static bool healthBoxSelected = false;
		Rectangle healthBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.2, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(healthBox, health, &healthBoxSelected);
		if (healthBoxSelected)
		{
			for (int i = 0; i < _amountSelectedNotes; i++)
			{
				_selectedNotes[i]->health = atoi(health);
				_selectedNotes[i]->health = (int)(fmin(fmax(_selectedNotes[i]->health, 0), 9));
			}
		}

		text = "health:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.2, tSize, WHITE);


		char hitSound[100] = {0};
		hitSound[0] = '\0';
		if (_selectedNotes[0]->hitSE_File != 0)
			snprintf(hitSound, 100, "%s", _selectedNotes[0]->hitSE_File);
		static bool hitSoundBoxSelected = false;
		Rectangle hitSoundBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.3, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
		textBox(hitSoundBox, hitSound, &hitSoundBoxSelected);
		if (strlen(hitSound) != 0)
		{
			if (_selectedNotes[0]->hitSE_File == 0)
				_selectedNotes[0]->hitSE_File = malloc(sizeof(char) * 100);
			strcpy(_selectedNotes[0]->hitSE_File, hitSound);
		}else if(_selectedNotes[0]->hitSE_File != 0)
		{
			free(_selectedNotes[0]->hitSE_File);
			_selectedNotes[0]->hitSE_File = 0;
		}

		text = "hitSound file:";
		tSize = GetScreenWidth() * 0.025;
		size = measureText(text, tSize);
		drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.3, tSize, WHITE);

		if (interactableButton("remove Animation", 0.025, GetScreenWidth() * 0.2, GetScreenHeight() * 0.7, GetScreenWidth() * 0.3, GetScreenHeight() * 0.07))
		{
			for(int i = 0; i < _amountSelectedNotes; i++)
			{
				if(_selectedNotes[i]->anim != 0)
					free(_selectedNotes[i]->anim);

				_selectedNotes[i]->anim = 0;
				_selectedNotes[i]->animSize = 0;
			}
		}
		
	}else
	{
		//animation :)
		static bool timeLineSelected = false;
		static float timeLine = 0;
		int value = timeLine * 100;
		
		if(_selectedNotes[0]->anim == 0)
		{
			_selectedNotes[0]->anim = malloc(sizeof(Frame) * 2);
			_selectedNotes[0]->anim[0].time = 0;
			_selectedNotes[0]->anim[0].vec = (Vector2){.x=1, .y=0.5};
			_selectedNotes[0]->anim[1].time = 1;
			_selectedNotes[0]->anim[1].vec = (Vector2){.x=0, .y=0.5};
			_selectedNotes[0]->animSize = 2;
		}

		slider((Rectangle){.x=0, .y=GetScreenHeight()*0.9, .width=GetScreenWidth(), .height=GetScreenHeight()*0.03}, &timeLineSelected, &value, 100, -100);
		timeLine = value / 100.0;
		drawNote(timeLine*_scrollSpeed+_selectedNotes[0]->time, _selectedNotes[0], WHITE);
		Frame * anim = _selectedNotes[0]->anim;
		for(int key = 0; key < _selectedNotes[0]->animSize; key++)
		{
			//draw keys
			if(interactableButton("k", 0.01, (anim[key].time)*GetScreenWidth()-GetScreenWidth()*0.01, GetScreenHeight()*0.85, GetScreenWidth()*0.02, GetScreenHeight()*0.05))
			{
				//delete key
				if(_selectedNotes[0]->animSize <= 2)
					break;
				_selectedNotes[0]->animSize --;
				for(int k = key; k < _selectedNotes[0]->animSize; k++)
				{
					anim[k].time = anim[k+1].time;
					anim[k].vec = anim[k+1].vec;
				}
				_selectedNotes[0]->anim = realloc(_selectedNotes[0]->anim, sizeof(Frame)*_selectedNotes[0]->animSize);
			}
		}
		if(IsMouseButtonReleased(0) && GetMouseY() < GetScreenHeight()*0.85)
		{
			int index = -1;
			for(int key = 0; key < _selectedNotes[0]->animSize; key++)
			{
				if((timeLine+1)/2 == _selectedNotes[0]->anim[key].time)
				{
					index = key;
					break;
				}
			}
			if(index != -1)
			{
				//modify existing frame
				anim[index].vec = (Vector2){.x=GetMouseX()/(float)GetScreenWidth(),.y=GetMouseY()/(float)GetScreenHeight()+0.05};
			}else
			{
				for(int key = 0; key < _selectedNotes[0]->animSize; key++) printf("%i  %i  %f %f %f\n", _selectedNotes[0]->animSize, key, _selectedNotes[0]->anim[key].time, _selectedNotes[0]->anim[key].vec.x, _selectedNotes[0]->anim[key].vec.y);
				//create new frame
				_selectedNotes[0]->animSize++;
				_selectedNotes[0]->anim = realloc(_selectedNotes[0]->anim, _selectedNotes[0]->animSize*sizeof(Frame));
				int newIndex = 0;
				for(int key = _selectedNotes[0]->animSize-2; key > 0; key--)
				{
					if(_selectedNotes[0]->anim[key].time > (timeLine+1)/2)
					{
						_selectedNotes[0]->anim[key+1] = _selectedNotes[0]->anim[key];
						newIndex = key;
					}
				}
				_selectedNotes[0]->anim[newIndex].time = (timeLine+1)/2;
				_selectedNotes[0]->anim[newIndex].vec = (Vector2){.x=GetMouseX()/(float)GetScreenWidth(),.y=GetMouseY()/(float)GetScreenHeight()+0.05};

				for(int key = 0; key < _selectedNotes[0]->animSize; key++) printf("%i  %i  %f %f %f\n", _selectedNotes[0]->animSize, key, _selectedNotes[0]->anim[key].time, _selectedNotes[0]->anim[key].vec.x, _selectedNotes[0]->anim[key].vec.y);
				
			}
		}
	}
}

void editorControls()
{
	float secondsPerBeat = (60.0/_map->bpm) / _barMeasureCount;
	if (!_musicPlaying)
	{
		//Snapping left and right with arrow keys
		if (IsKeyPressed(KEY_RIGHT))
		{
			double before = _musicHead;
			// Snap to closest beat
			_musicHead = roundf((getMusicHead() - _map->offset/1000.0) / secondsPerBeat) * secondsPerBeat;
			// Add the offset
			_musicHead += _map->offset / 1000.0;
			// Add the bps to the music head
			if(before >= _musicHead-0.001) _musicHead += secondsPerBeat;
			if(before >= _musicHead-0.001) _musicHead += secondsPerBeat;
			// // snap it again (it's close enough right?????)
			// _musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
			_musicPlaying = false;
		}

		if (IsKeyPressed(KEY_LEFT))
		{
			double before = _musicHead;
			_musicHead = floorf((getMusicHead() - _map->offset/1000.0) / secondsPerBeat) * secondsPerBeat;
			_musicHead += _map->offset / 1000.0;	
			printf("before %.2f  musichead %.2f\n", before, _musicHead);
			if(before <= _musicHead+0.001)
				_musicHead -= secondsPerBeat;
			if(before <= _musicHead+0.001)
				_musicHead -= secondsPerBeat;
			// _musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
			_musicPlaying = false;
		}

		//Scroll timeline with mousewheel
		if (GetMouseWheelMove() > 0)
			_musicHead += GetFrameTime() * (_scrollSpeed * 6);
		if (GetMouseWheelMove() < 0)
			_musicHead -= GetFrameTime() * (_scrollSpeed * 6);
		if (IsMouseButtonDown(2))
		{
			_musicHead -= GetMouseDelta().x / GetScreenWidth() * _scrollSpeed;
		}
		//Pause menu
		if (IsKeyPressed(KEY_ESCAPE))
		{
			_pGameplayFunction = &fPause;
			_pNextGameplayFunction = &fEditor;
			free(_selectedNotes);
			_selectedNotes = 0;
			_amountSelectedNotes = 0;
			return;
		}

		//Small optimisation defined in main.c
	if (_isKeyPressed)
	{
		float closestTime = 55;
		int closestIndex = 0;
		for (int i = 0; i < _amountNotes; i++)
		{
			if (closestTime > fabs(_papNotes[i]->time - getMusicHead()))
			{
				closestTime = fabs(_papNotes[i]->time - getMusicHead());
				closestIndex = i;
			}
		}
		if (IsKeyPressed(KEY_Z) && IsKeyDown(KEY_LEFT_CONTROL))
		{
			undo();
		}else if (IsKeyPressed(KEY_Z) && !IsKeyDown(KEY_LEFT_CONTROL) && closestTime > 0.03f)
		{
			doAction();
			newNote(getMusicHead());
			_noteIndex = closestIndex;
		}

		if (IsKeyPressed(KEY_A))
		{
			doAction();
			float pos = roundf((getMusicHead() - _map->offset/1000.0) / secondsPerBeat) * secondsPerBeat + _map->offset/1000.0;
			newNote(pos);
			_noteIndex = closestIndex;
		}


		bool delKey = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_DELETE);
		if (delKey && closestTime < _maxMargin && !_amountSelectedNotes)
		{
			doAction();
			removeNote(closestIndex);
			if(closestIndex >= _noteIndex)
				_noteIndex--;
		}else if(delKey)
		{
			while(_amountSelectedNotes > 0)
			{
				removeNote(findClosestNote(_papNotes, _amountNotes, _selectedNotes[0]->time));
			}
			free(_selectedNotes);
			_selectedNotes = 0;
			_amountSelectedNotes = 0;
		}

		if (IsKeyPressed(KEY_C) && !_musicPlaying)
		{
			_musicHead = roundf((getMusicHead() - _map->offset/1000.0) / secondsPerBeat) * secondsPerBeat + _map->offset/1000.0;
		}

		if (IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_CONTROL) && closestTime > 0.03f && _amountSelectedNotes > 0)
		{
			doAction();
			//copy currently selected notes at _musichead position

			//get begin point of notes
			float firstNote = -1;
			for(int i = 0; i < _amountSelectedNotes; i++)
			{
				if(firstNote == -1)
					firstNote = _selectedNotes[i]->time;
				else if(_selectedNotes[i]->time < firstNote)
				{
					firstNote = _selectedNotes[i]->time;
				}
			}
			//copy over notes
			for(int i = 0; i < _amountSelectedNotes; i++)
			{
				int note = newNote(_musicHead+_selectedNotes[i]->time-firstNote);
				if(_selectedNotes[i]->anim)
				{
					_papNotes[note]->anim = malloc(sizeof(Frame)*_selectedNotes[i]->animSize);
					for(int j = 0; j < _selectedNotes[i]->animSize; j++)
					{
						_papNotes[note]->anim[j] = _selectedNotes[i]->anim[j];
					}
					_papNotes[note]->animSize = _selectedNotes[i]->animSize;
				}

				if(_selectedNotes[i]->hitSE_File)
				{
					_papNotes[note]->hitSE_File = malloc(100);
					strcpy(_papNotes[note]->hitSE_File, _selectedNotes[i]->hitSE_File);
					char tmpStr[100];
					snprintf(tmpStr, 100, "maps/%s/%s", _map->folder, _selectedNotes[i]->hitSE_File);
					_papNotes[note]->custSound = addCustomSound(tmpStr);
				}

				if(_selectedNotes[i]->texture_File)
				{
					_papNotes[note]->texture_File = malloc(100);
					strcpy(_papNotes[note]->texture_File, _selectedNotes[i]->texture_File);
					char tmpStr[100];
					snprintf(tmpStr, 100, "maps/%s/%s", _map->folder, _selectedNotes[i]->texture_File);
					_papNotes[note]->custTex = addCustomTexture(tmpStr);
				}
			}
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
	
	//Change scrollspeed
	if (IsKeyPressed(KEY_UP) || (GetMouseWheelMove() > 0 && IsKeyDown(KEY_LEFT_CONTROL)))
		_scrollSpeed *= 1.2;
	if (IsKeyPressed(KEY_DOWN) || (GetMouseWheelMove() < 0 && IsKeyDown(KEY_LEFT_CONTROL)))
		_scrollSpeed /= 1.2;
	if (_scrollSpeed == 0)
		_scrollSpeed = 0.01;
	
	//Selecting notes
	if (IsMouseButtonPressed(0) && GetMouseY() > GetScreenHeight() * 0.3 && GetMouseY() < GetScreenHeight() * 0.6)
	{
		if(!IsKeyDown(KEY_LEFT_SHIFT))
		{
			int note = findClosestNote(_papNotes, _amountNotes, screenToMusicTime(GetMouseX()));
			bool unselect = false;
			if(_amountSelectedNotes == 1 && _selectedNotes[0] == _papNotes[note])
				unselect = true;
			free(_selectedNotes);
			_amountSelectedNotes = 0;
			_selectedNotes = 0;
			if(!unselect)
				addSelectNote(findClosestNote(_papNotes, _amountNotes, screenToMusicTime(GetMouseX())));
		}
		else
			addSelectNote(findClosestNote(_papNotes, _amountNotes, screenToMusicTime(GetMouseX())));
	}
	//prevent bugs by setting musicHead to 0 when it gets below 0
	if (getMusicHead() < 0)
		_musicHead = 0;
	//prevent bugs by setting muiscHead to the max song duration
	if (getMusicHead() > getMusicDuration())
		_musicHead = getMusicDuration();

	if (IsKeyPressed(KEY_SPACE))
	{
		_musicPlaying = !_musicPlaying;
		if (!_musicPlaying && floorf(getMusicHead() / secondsPerBeat) * secondsPerBeat < getMusicDuration())
		{
			_musicHead = floorf((getMusicHead()-_map->offset/1000.0) / secondsPerBeat) * secondsPerBeat;
			_musicHead += _map->offset/1000.0;
		}
		_noteIndex = findClosestNote(_papNotes, _amountNotes, _musicHead);
		
	}
}

void fEditor()
{
	float secondsPerBeat = (60.0/_map->bpm) / _barMeasureCount;
	if (_musicPlaying)
	{
		_musicHead += GetFrameTime() * _musicSpeed;
		if (_amountNotes > 0 && _noteIndex < _amountNotes && getMusicHead() > _papNotes[_noteIndex]->time)
		{
			if(_papNotes[_noteIndex]->custSound)
				playAudioEffect(_papNotes[_noteIndex]->custSound->sound, _papNotes[_noteIndex]->custSound->length);
			else
				playAudioEffect(_pHitSE, _hitSE_Size);

			_noteIndex++;
			while(_noteIndex < _amountNotes -1 && getMusicHead() > _papNotes[_noteIndex]->time)
			{
				_noteIndex++;
			}
		}

		while(_noteIndex > 0 && getMusicHead() < _papNotes[_noteIndex-1]->time)
			_noteIndex--;
		
		if (endOfMusic())
		{
			_musicPlaying = false;
			_musicPlaying = false;
		}
	}else
	{
		setMusicFrameCount();
		_noteIndex = 0;
	}

	if (!_showSettings && !_showNoteSettings)
	{
		editorControls();
	}
	
	if(_musicHead < 0)
		_musicHead = 0;
	if(_musicHead > getMusicDuration())
		_musicHead = getMusicDuration();

	
	ClearBackground(BLACK);

	drawBackground();

	// draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() / 2;

	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	dNotes();
	drawVignette();
	drawMusicGraph(0.4);
	drawBars();
	if(drawProgressBarI(!_showNoteSettings && !_showSettings))
		_musicPlaying = false;

	editorSettings();

	if (_amountSelectedNotes > 0)
	{
		if (_showNoteSettings)
		{
			editorNoteSettings();
		}
		else if (interactableButton("Note settings", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.15, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
		{
			_showNoteSettings = !_showNoteSettings;
		}
	}
	if (!_showNoteSettings)
	{
		if ( (_showSettings && IsKeyPressed(KEY_ESCAPE)) ||
			interactableButton("Song settings", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.05, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
		{
			_showSettings = !_showSettings;
		}
		for(int i = 0; i < _amountSelectedNotes; i++)
		{
			DrawCircle(musicTimeToScreen(_selectedNotes[i]->time), GetScreenHeight()*0.55, GetScreenWidth()*0.013, BLACK);
			DrawCircle(musicTimeToScreen(_selectedNotes[i]->time), GetScreenHeight()*0.55, GetScreenWidth()*0.01, WHITE);
		}
	}

	if(!_showNoteSettings && !_showSettings)
	{
		// Speed slider
		static bool speedSlider = false;
		int speed = _musicSpeed * 4;
		slider((Rectangle){.x = GetScreenWidth() * 0.1, .y = GetScreenHeight() * 0.05, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.03}, &speedSlider, &speed, 8, 1);
		_musicSpeed = speed / 4.0;
		if (interactableButton("reset", 0.03, GetScreenWidth() * 0.32, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
			_musicSpeed = 1;
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
		{
			saveScore();
			int tmp = 0;
			// submitScore(_map->id, _score, &tmp);
		}
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
		DrawRing((Vector2){.x = musicTimeToScreen(_musicHead), .y = GetScreenHeight() * 0.5}, size * GetScreenWidth() * 0.001, size * 0.7 * GetScreenWidth() * 0.001, 0, 360, 50, ColorAlpha(WHITE, rippleEffectStrength[i] * 0.35));
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

	if (_noteIndex < _amountNotes-1 && getMusicHead() - _maxMargin > _papNotes[_noteIndex]->time)
	{
		// passed note
		_noteIndex++;
		feedback("miss!", 1.3 - _health / 100);
		_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
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
			if (closestNote > _papNotes[i]->time - getMusicHead() - _maxMargin)
			{
				closestNote = _papNotes[i]->time - getMusicHead() - _maxMargin;
				closestTime = fabs(_papNotes[i]->time - getMusicHead());
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
				_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
				_notesMissed++;
			}
			_averageAccuracy = ((_averageAccuracy * (_noteIndex - _notesMissed)) + ((1 / _maxMargin) * closestTime)) / (_noteIndex - _notesMissed + 1);
			// _averageAccuracy = 0.5/_amountNotes;
			int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / _maxMargin * getMarginMod())) * _papNotes[_noteIndex]->health;
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
			_papNotes[_noteIndex]->hit = 1;
			_noteIndex++;
			_combo++;
			if(_papNotes[closestIndex]->custSound)
				playAudioEffect(_papNotes[closestIndex]->custSound->sound, _papNotes[closestIndex]->custSound->length);
			else	
				playAudioEffect(_pHitSE, _hitSE_Size);
		}
		else
		{
			printf("missed note\n");
			feedback("miss!", 1.3 - _health / 100);
			_combo = 0;
			_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
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
	snprintf(tmpString, 20, "score: %i", _score);
	drawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight() * 0.05, GetScreenWidth() * 0.05, WHITE);

	// draw combo
	snprintf(tmpString, 20, "combo: %i", _combo);
	drawText(tmpString, GetScreenWidth() * 0.70, GetScreenHeight() * 0.05, GetScreenWidth() * 0.05, WHITE);

	// draw acc
	//  sprintf(tmpString, "acc: %.5f", (int)(100*_averageAccuracy*(_amountNotes/(_noteIndex+1))));
	//  printf("%.2f   %.2f\n", _averageAccuracy, ((float)_amountNotes/(_noteIndex+1)));
	snprintf(tmpString, 20, "acc: %.2f", 100 * (1 - _averageAccuracy));
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
	snprintf(tmpString, 9, "%i", _score);
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
	int amount;
	static int oldAmount = 0;
	char **files = GetDirectoryFiles("maps/", &amount);

	int newAmount = 0;
	for(int i = 0; i < amount; i++)
	{
		char str [100];
		snprintf(str, 100, "maps/%s/map.data", files[i]);
		if(FileExists(str))
		{
			newAmount++;
		}
	}
	char **newFiles = malloc(sizeof(char *) * newAmount);
	for(int i = 0, j = 0; i < amount && j < newAmount; i++)
	{
		char str [100];
		snprintf(str, 100, "maps/%s/map.data", files[i]);
		if(FileExists(str))
		{
			newFiles[j] = malloc(sizeof(char) * strlen(files[i]));
			strcpy(newFiles[j], files[i]);
			j++;
		}
		// free(files[i]);
	}
	// free(files);
	ClearDirectoryFiles();
	files = newFiles;
	amount = newAmount;

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
			tmp2[i] = _pMaps[i];
		}
		for (int i = amount; i < oldAmount; i++)
			freeMap(&_pMaps[i]);
		free(_pMaps);
		_pMaps = tmp2;
	}
	else
	{
		*args->highScores = malloc(amount * sizeof(int));
		*args->combos = malloc(amount * sizeof(int));
		*args->accuracy = malloc(amount * sizeof(float));
		filesCaching = calloc(amount, sizeof(char *));
		for (int i = 0; i < amount; i++)
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
		//todo reenable cache when memory leak is fixed and music isn't unloaded
		for (int j = 0; j < amount; j++)
		{
			if (!filesCaching[j][0])
				continue;
			if (strncmp(filesCaching[j], files[i], 100) == 0)
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
		printf("cache miss %s %i\n", files[i], mapIndex);
		// cache miss

		freeMap(&_pMaps[mapIndex]);
		_pMaps[mapIndex] = loadMapInfo(files[i]);
		if(_pMaps[mapIndex].name == 0)
		{
			printf("skipping map, failed to load\n");
			continue;
		}
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
	snprintf(str, 100, "maps/%s/%s", map->folder, map->imageFile);
	img = LoadImage(str);
	map->cpuImage = img;
	if(img.width==0) //failed to load so setting it back to -1
		map->cpuImage.width=-2;
	// lockLoadingMutex();
	// _loading--;
	// unlockLoadingMutex();
}

#include <unistd.h>

void fMapSelect()
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
			if (interactableButton(_mods[i].name, 0.03, GetScreenWidth() * (0.2 + x * 0.22), GetScreenHeight() * (0.6 + y * 0.12), GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
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

	textBox((Rectangle){.x = GetScreenWidth() * 0.35, .y = GetScreenHeight() * 0.05, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.05}, search, &searchSelected);

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
			_musicFrameCount = _pMaps[hoverMap].musicPreviewOffset * 48000 * 2;
	}
	// draw map button
	Rectangle mapSelectRect = (Rectangle){.x = 0, .y = GetScreenHeight() * 0.13, .width = GetScreenWidth(), .height = GetScreenHeight()};
	BeginScissorMode(mapSelectRect.x, mapSelectRect.y, mapSelectRect.width, mapSelectRect.height);
	int mapCount = -1;
	for (int i = 0; i < amount; i++)
	{
		// draw maps
		if (search[0] != '\0')
		{
			bool missingLetter = false;
			char str[100];
			snprintf(str, 100, "%s - %s", _pMaps[i].name, _pMaps[i].artist);
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
		int x = GetScreenWidth() * 0.02 + GetScreenWidth() * 0.32 * (mapCount % 3);
		Rectangle mapButton = (Rectangle){.x = x, .y = menuScrollSmooth * GetScreenHeight() + GetScreenHeight() * ((floor(i / 3) > floor(selectedMap / 3) && selectedMap != -1 ? 0.3 : 0.225) + 0.3375 * floor(mapCount / 3)), .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.3};
		if ((mouseInRect(mapButton) || selectedMap == i) && mouseInRect(mapSelectRect))
		{
			if (hoverPeriod > 1 && hoverPeriod < 2)
			{
				// play music
				char str[100];
				loadMusic(&_pMaps[i]);
				_playMenuMusic = false;
				_musicFrameCount = _pMaps[i].musicPreviewOffset * 48000 * 2;
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
					// printf("map image width %i\n", _pMaps[i].cpuImage.width);
		if(_pMaps[i].cpuImage.width == 0)
		{
			_pMaps[i].cpuImage.width = -1;
			createThread((void *(*)(void *))loadMapImage, &_pMaps[i]);
			_pMaps[i].cpuImage.width = -1;
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
				// 		printf("%i\n", i);
				// 	}

				// 	if(_map->cpuImage.width < 1)
				// 		_map->image = _background;
				// }
			}
			drawMapThumbnail(mapButton, &_pMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], true);
			if (interactableButtonNoSprite("play", 0.0225, mapButton.x, mapButton.y + mapButton.height, mapButton.width * (1 / 3.0) * 1.01, mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect))
			{
				_pNextGameplayFunction = &fPlaying;
				_pGameplayFunction = &fCountDown;
			}
			if (interactableButtonNoSprite("editor", 0.0225, mapButton.x + mapButton.width * (1 / 3.0), mapButton.y + mapButton.height, mapButton.width * (1 / 3.0) * 1.01, mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect))
			{
				_pNextGameplayFunction = &fEditor;
				_pGameplayFunction = &fEditor;
				for(int j = 0; j < UNDOBUFFER; j++)
				{
					if(_paUndoBuffer[j])
					{
						for(int k = 0; k < _undoBufferSize[j]; k++)
						{
							freeArray(_paUndoBuffer[j][k].anim);
							freeArray(_paUndoBuffer[j][k].hitSE_File);
							freeArray(_paUndoBuffer[j][k].texture_File);
						}
						free(_paUndoBuffer[j]);
					}
					_paUndoBuffer[j] = malloc(_amountNotes*sizeof(Note));
					

					//copy everything over
					for(int k = 0; k < _amountNotes; k++)
					{
						_paUndoBuffer[j][k] = *_papNotes[k];
						_paUndoBuffer[j][k].custSound = 0;
						_paUndoBuffer[j][k].custTex = 0;

						if(_paUndoBuffer[j][k].anim)//copy over animations
						{
							_paUndoBuffer[j][k].anim = malloc(sizeof(Frame)*_paUndoBuffer[j][k].animSize);
							for(int l = 0; l < _paUndoBuffer[j][k].animSize; l++)
								_paUndoBuffer[j][k].anim[l] = _papNotes[k]->anim[l];
						}
						char fullPath[100];
						if(_papNotes[k]->hitSE_File)
						{
							snprintf(fullPath, 100, "maps/%s/%s", _map->folder, _papNotes[k]->hitSE_File);
							_paUndoBuffer[j][k].custSound = addCustomSound(fullPath);
							_paUndoBuffer[j][k].hitSE_File = malloc(100);
							strcpy(_paUndoBuffer[j][k].hitSE_File, _papNotes[k]->hitSE_File);
						}
						if(_papNotes[k]->texture_File)
						{
							snprintf(fullPath, 100, "maps/%s/%s", _map->folder, _papNotes[k]->texture_File);
							_paUndoBuffer[j][k].custTex = addCustomTexture(fullPath);
							_paUndoBuffer[j][k].texture_File = malloc(100);
							strcpy(_paUndoBuffer[j][k].texture_File, _papNotes[k]->texture_File);
						}
					}
					// memcpy(_paUndoBuffer[j], _papNotes, _amountNotes*sizeof(Note));
					_undoBufferSize[j] = _amountNotes;
				}
				startMusic();
				_musicPlaying = false;
			}
			if (interactableButtonNoSprite("export", 0.0225, mapButton.x + mapButton.width * (1 / 3.0 * 2), mapButton.y + mapButton.height, mapButton.width * (1 / 3.0), mapButton.height * 0.15 * selectMapTransition) && mouseInRect(mapSelectRect))
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
		if(_pMaps[selMap].name != 0)
		{
			char str[100];
			// printf("%p\n", _pMaps[selMap].name);
			strcpy(str, _pMaps[selMap].name);
			strcat(str, " - ");
			strcat(str, _pMaps[selMap].artist);
			int textSize = measureText(str, GetScreenWidth() * 0.05);
			drawText(str, GetScreenWidth() * 0.9 - textSize, GetScreenHeight() * 0.92, GetScreenWidth() * 0.05, WHITE);
		}
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
		strcpy(newMap.artist, "Artist");
		newMap.mapCreator = malloc(100);
		newMap.mapCreator[0] = '\0';
		strcpy(newMap.mapCreator, "mapCreator");
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
	Rectangle nameBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.375, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(nameBox, newMap.name, &nameBoxSelected);
	drawText("name", middle, GetScreenHeight() * 0.325, GetScreenHeight() * 0.05, WHITE);

	static bool artistBoxSelected = false;
	Rectangle artistBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.5, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(artistBox, newMap.artist, &artistBoxSelected);
	drawText("artist", middle, GetScreenHeight() * 0.45, GetScreenHeight() * 0.05, WHITE);

	static bool creatorBoxSelected = false;
	Rectangle creatorBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.625, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(creatorBox, newMap.mapCreator, &creatorBoxSelected);
	drawText("creator", middle, GetScreenHeight() * 0.575, GetScreenHeight() * 0.05, WHITE);

	char str[100] = {'\0'};
	if (newMap.bpm != 0)
		snprintf(str, 100, "%i", newMap.bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.875, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(bpmBox, str, &bpmBoxSelected);
	newMap.bpm = fmin(fmax(atoi(str), 0), 500);
	drawText("bpm", middle, GetScreenHeight() * 0.825, GetScreenHeight() * 0.05, WHITE);

	static bool difficultyBoxSelected = false;
	Rectangle difficultyBox = (Rectangle){.x = middle, .y = GetScreenHeight() * 0.75, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
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