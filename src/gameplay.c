
#include "gameplay.h"
#include "files.h"
#include "drawing.h"
#include "audio.h"

#include "deps/raylib/src/raylib.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Texture2D _noteTex, _background, _heartTex, _healthBarTex;
extern Color _fade;
extern bool _musicPlaying, _musicLoops;
extern float _musicHead, _transition;
extern void * _pHitSE, *_pMissHitSE, *_pMissSE, *_pButtonSE;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size, _buttonSE_Size, _musicFrameCount, _musicLength;
extern bool _isKeyPressed;

extern void *_pFailSE;
extern int _failSE_Size;

extern void *_pFinishSE;
extern int _finishSE_Size;


extern Map _pMaps[100];

float _scrollSpeed = 0.6;
int _noteIndex = 0, _amountNotes =0 ;
bool _noBackground = false;
float _health = 50;
int _score= 0, _highScore, _combo = 0, _highestCombo, _highScoreCombo = 0;
float _maxMargin = 0.1;
int _hitPoints = 5;
int _missPenalty = 10;
bool _mapRefresh = true;
Map * _map;
Settings _settings = (Settings){.volumeGlobal=100, .volumeMusic=100, .volumeSoundEffects=100, .zoom=7, .offset=0};

//Timestamp of all the notes
float * _pNotes;
void (*_pNextGameplayFunction)();
void (*_pGameplayFunction)();

bool checkFileDropped()
{
	if(IsFileDropped())
	{
		int amount = 0;
		char ** files = GetDroppedFiles(&amount);
		for(int i = 0; i < amount; i++)
		{
			if(strcmp(GetFileExtension(files[i]), ".zip") == 0)
			{
				addZipMap(files[i]);
			}
		}
		ClearDroppedFiles();
	}
}

void gotoMainMenu(bool mainOrSelect)
{
	stopMusic();
	loadMusic("assets/menuMusic.mp3");
	_musicPlaying = true;
	randomMusicPoint();
	if(mainOrSelect)
		_pGameplayFunction = &fMainMenu;
	else _pGameplayFunction = &fMapSelect;
	
	resetBackGround();
}

float getMusicHead()
{
	if(_musicPlaying)
		return _musicHead - _settings.offset;
	else return _musicHead;
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

void textBox(Rectangle rect, char * str, bool * selected)
{
	drawButton(rect, str, 0.03);
	if(mouseInRect(rect) && IsMouseButtonDown(0) || *selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while(c != 0)
		{
			for(int i = 0; i < 99; i++)
			{
				if(str[i] == '\0')
				{
					str[i] = c;
					str[i+1] = '\0';
					break;
				}
			}
			c = GetCharPressed();
		}
		if(IsKeyPressed(KEY_BACKSPACE))
		{
			for(int i = 1; i < 99; i++)
			{
				if(str[i] == '\0')
				{
					str[i-1] = '\0';
					break;
				}
			}
		}
	}
	if(*selected)
		DrawRectangle(rect.x+rect.width*0.2, rect.y+rect.height*0.75, rect.width*0.6, rect.height*0.1, DARKGRAY);

	if (!mouseInRect(rect) && IsMouseButtonDown(0))
		*selected = false;
}

void numberBox(Rectangle rect, int * number, bool * selected)
{
	char str [10];
	sprintf(str, "%i", *number);
	drawButton(rect, str, 0.03);
	if(mouseInRect(rect) && IsMouseButtonDown(0) || *selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while(c != 0)
		{
			*number = c - '0';
			c = GetCharPressed();
		}
	}
	if(*selected)
		DrawRectangle(rect.x+rect.width*0.2, rect.y+rect.height*0.75, rect.width*0.6, rect.height*0.1, DARKGRAY);

	if (!mouseInRect(rect) && IsMouseButtonDown(0))
		*selected = false;
}


void slider(Rectangle rect, bool * selected, int * value, int max, int min)
{
	if(*value > max)
		*value = max;
	if(*value < min)
		*value = min;

	Color color = WHITE;
	if(*selected)
		color = LIGHTGRAY;
	
	float sliderMapped = (*value-min) / (float)(max-min);
	DrawRectangle(rect.x, rect.y, rect.width, rect.height, WHITE);
	DrawCircle(rect.x+rect.width*sliderMapped, rect.y+rect.height*0.5, rect.height, color);

	if((mouseInRect(rect) || *selected )&& IsMouseButtonDown(0))
	{
		*selected = true;
		*value = ((GetMouseX()-rect.x)/rect.width)*(max-min)+min;
	}

	if(!IsMouseButtonDown(0))
		*selected = false;
}

bool interactableButton(char * text, float fontScale, float x, float y,float width,float height) {
	Rectangle button = (Rectangle){.x=x, .y=y, .width=width, .height=height};
	drawButton(button,text,fontScale);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		return true;
	}
	return false;
}

void removeNote(int index)
{
	_amountNotes--;
	float *tmp = malloc(sizeof(float) * _amountNotes);
	memcpy(tmp, _pNotes, sizeof(float) * _amountNotes);
	for (int i = index; i < _amountNotes; i++)
	{
		tmp[i] = _pNotes[i + 1];
	}
	free(_pNotes);
	_pNotes = tmp;
}

void newNote(float time)
{
	printf("new note time %f\n", time);
	int closestIndex = 0;
	_amountNotes++;
	float *tmp = calloc(_amountNotes, sizeof(float));
	for (int i = 0; i < _amountNotes - 1; i++)
	{
		tmp[i] = _pNotes[i];
		if (tmp[i] < time)
		{
			printf("found new closest :%i   value %.2f musicHead %.2f\n", i, tmp[i], time);
			closestIndex = i + 1;
		}
	}
	for (int i = closestIndex; i < _amountNotes - 1; i++)
	{
		tmp[i + 1] = _pNotes[i];
	}

	free(_pNotes);
	_pNotes = tmp;
	_pNotes[closestIndex] = time;
}

void fPause()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();

	dNotes();
	drawVignette();
	drawProgressBar();
	DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

	//TODO dynamically change seperation depending on the amount of buttons?
	float middle = GetScreenWidth()/2;

	if(IsKeyPressed(KEY_ESCAPE) || interactableButton("Continue", 0.05,middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.3, GetScreenWidth()*0.3,GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		if(_pNextGameplayFunction == &fPlaying)
			_pGameplayFunction = &fCountDown;
		else
			_pGameplayFunction = _pNextGameplayFunction;
	}
	if (_pNextGameplayFunction == &fEditor && interactableButton("Save & Exit", 0.05, middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.5, GetScreenWidth()*0.3,GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		saveFile(_amountNotes);
		gotoMainMenu(false);
	}
	
	if(interactableButton("Exit", 0.05, middle - GetScreenWidth()*0.15, _pNextGameplayFunction == &fEditor ? GetScreenHeight() * 0.7 : GetScreenHeight() * 0.5, GetScreenWidth()*0.3,GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		unloadMap();
		gotoMainMenu(false);
	}
	drawCursor();
}

void fCountDown ()
{
	_musicLoops = false;
	_musicPlaying = false;
	static float countDown  = 0;
	static bool contin = false;
	if(countDown == 0) countDown = GetTime() + 3;
	if(countDown - GetTime() +GetFrameTime() <= 0)
	{
		countDown = 0;
		_pGameplayFunction = _pNextGameplayFunction;
		if(!contin)
		{
			//switching to playing map
			printf("switching to playing map! \n");
			startMusic();
			
			_health = 50;
			_combo = 0;
			_score = 0;
			_noteIndex =1;
			_musicHead = 0;
			contin = false;
			_scrollSpeed = 4.2/_map->zoom;
			if(_settings.zoom != 0)
				_scrollSpeed = 4.2/_settings.zoom;
		}
		return;
	}
	if(getMusicHead() <= 0) _musicHead = GetTime()-countDown;
	else contin = true;
	ClearBackground(BLACK);
	drawBackground();
	
	//draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;

	dNotes();
	
	
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=255/2});

	//draw score
	char * tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	drawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
	
	drawCursor();
	
	float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale,
		.y=GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale,WHITE);
	DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

	sprintf(tmpString, "%i", (int)(countDown - GetTime() + 1));
	float textSize = measureText(tmpString, GetScreenWidth() * 0.3);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.3, GetScreenWidth() * 0.3, WHITE);
	free(tmpString);
	drawVignette();
}

void fMainMenu()
{
	checkFileDropped();
	_musicLoops = true;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth()/2;
	drawVignette();
	//draw main menu
	
	
	
	
	
	
	//Rectangle playButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.3, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	// drawButton(playButton,"play", 0.04);
	// Rectangle editorButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.45, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	// drawButton(editorButton,"Editor", 0.04);
	// Rectangle SettingsButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.60, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	// drawButton(SettingsButton,"Settings", 0.04);
	// Rectangle recordingButton = (Rectangle){.x=middle - GetScreenWidth()*0.10, .y=GetScreenHeight() * 0.75, .width=GetScreenWidth()*0.2,.height=GetScreenHeight()*0.08};
	// drawButton(recordingButton,"Record", 0.04);

	
	

	if(interactableButton("Play", 0.04, middle - GetScreenWidth()*0.3,GetScreenHeight() * 0.3,GetScreenWidth()*0.2,GetScreenHeight()*0.08))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//switching to playing map
		printf("switching to playing map!\n");
		_pNextGameplayFunction = &fPlaying;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if(interactableButton("Editor", 0.04, middle - GetScreenWidth()*0.32,GetScreenHeight() * 0.45,GetScreenWidth()*0.2,GetScreenHeight()*0.08))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//switching to editing map
		_health = 50;
		_score = 0;
		_noteIndex =0;
		_amountNotes = 0;
		_musicHead = 0;
		printf("switching to editor map!\n");
		setMusicStart();
		_pNextGameplayFunction = &fEditor;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if (interactableButton("Settings", 0.04, middle - GetScreenWidth()*0.34,GetScreenHeight() * 0.60,GetScreenWidth()*0.2,GetScreenHeight()*0.08))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//Switching to settings
		_pGameplayFunction = &fSettings;
		_transition = 0.1;
		_settings.offset = _settings.offset*1000;
	}

	if(interactableButton("Recording", 0.035, middle - GetScreenWidth()*0.36,GetScreenHeight() * 0.75,GetScreenWidth()*0.2,GetScreenHeight()*0.08))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//switching to recording map
		_health = 50;
		_score = 0;
		_noteIndex =0;
		_amountNotes = 0;
		_musicHead = 0;
		_pNotes = calloc(sizeof(float), 1);
		printf("switching to recording map! \n");
		_pNextGameplayFunction = &fRecording;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if (interactableButton("Export", 0.04, middle - GetScreenWidth()*0.38,GetScreenHeight() * 0.90,GetScreenWidth()*0.2,GetScreenHeight()*0.08))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//Switching to export
		_pNextGameplayFunction = &fExport;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}
	

	//gigantic ass title 
	char * title = "One Button Rhythm";
	float tSize = GetScreenWidth()*0.05;
	int size = MeasureText(title, tSize);
	//dropshadow
	drawText(title, middle-size/2+GetScreenWidth()*0.004, GetScreenHeight()*0.107, tSize, DARKGRAY);
	//real title
	drawText(title, middle-size/2, GetScreenHeight()*0.1, tSize, WHITE);

	drawCursor();

}

void fSettings() {
	_musicPlaying = false;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
	int middle = GetScreenWidth()/2;
	
	//gigantic ass settings title 
	char * title = "Settings";
	float tSize = GetScreenWidth()*0.05;
	int size = MeasureText(title, tSize);
	//dropshadow
	drawText(title, middle-size/2+GetScreenWidth()*0.004, GetScreenHeight()*0.107, tSize, DARKGRAY);
	//real title
	drawText(title, middle-size/2, GetScreenHeight()*0.1, tSize, WHITE);

	char zoom[10] = {0};
	if(_settings.zoom != 0)
		sprintf(zoom, "%i", _settings.zoom);
	static bool zoomBoxSelected = false;

	Rectangle zoomBox = (Rectangle){.x=GetScreenWidth()*0.4, .y=GetScreenHeight()*0.7, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(zoomBox, zoom, &zoomBoxSelected);

	_settings.zoom=atoi(zoom);
	_settings.zoom = fmin(fmax(_settings.zoom, 0), 300);
	tSize = GetScreenWidth()*0.03;
	size = MeasureText("zoom", tSize);
	drawText("zoom", zoomBox.x+zoomBox.width/2-size/2, zoomBox.y-GetScreenHeight()*0.05, tSize, WHITE);

	char offset[10] = {0};
	if(_settings.offset != 0)
		sprintf(offset, "%i", (int)_settings.offset);
	static bool offsetBoxSelected = false;
	Rectangle offsetBox = (Rectangle){.x=GetScreenWidth()*0.4, .y=GetScreenHeight()*0.85, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(offsetBox, offset, &offsetBoxSelected);
	_settings.offset = (float)atoi(offset);
	_settings.offset = fmin(fmax(_settings.offset, 0), 300);
	tSize = GetScreenWidth()*0.03;
	size = MeasureText("offset", tSize);
	drawText("offset", offsetBox.x+offsetBox.width/2-size/2, offsetBox.y-GetScreenHeight()*0.05, tSize, WHITE);


	static bool gvBoolSelected = false; 
	Rectangle gvSlider = (Rectangle){.x=GetScreenWidth()*0.35, .y=GetScreenHeight()*0.3, .width=GetScreenWidth()*0.3, .height=GetScreenHeight()*0.03};
	slider(gvSlider, &gvBoolSelected, &_settings.volumeGlobal, 100, 0);
	tSize = GetScreenWidth()*0.03;
	size = MeasureText("global volume", tSize);
	drawText("global volume", gvSlider.x+gvSlider.width/2-size/2, gvSlider.y-GetScreenHeight()*0.05, tSize, WHITE);

	static bool mvBoolSelected = false; 
	Rectangle mvSlider = (Rectangle){.x=GetScreenWidth()*0.35, .y=GetScreenHeight()*0.45, .width=GetScreenWidth()*0.3, .height=GetScreenHeight()*0.03};
	slider(mvSlider, &mvBoolSelected, &_settings.volumeMusic, 100, 0);
	tSize = GetScreenWidth()*0.03;
	size = MeasureText("music volume", tSize);
	drawText("music volume", mvSlider.x+mvSlider.width/2-size/2, mvSlider.y-GetScreenHeight()*0.05, tSize, WHITE);

	static bool aevBoolSelected = false; 
	Rectangle aevSlider = (Rectangle){.x=GetScreenWidth()*0.35, .y=GetScreenHeight()*0.6, .width=GetScreenWidth()*0.3, .height=GetScreenHeight()*0.03};
	slider(aevSlider, &aevBoolSelected, &_settings.volumeSoundEffects, 100, 0);
	tSize = GetScreenWidth()*0.03;
	size = MeasureText("sound sffect volume", tSize);
	drawText("sound Effect volume", aevSlider.x+aevSlider.width/2-size/2, aevSlider.y-GetScreenHeight()*0.05, tSize, WHITE);

	if(interactableButton("Back", 0.03,GetScreenWidth()*0.05,GetScreenHeight()*0.05,GetScreenWidth()*0.1,GetScreenHeight()*0.05))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		_pGameplayFunction=&fMainMenu;
		_transition = 0.1;
		_settings.offset = _settings.offset*0.001;
		saveSettings();
		return;
	}

	drawCursor();
}

void fEndScreen ()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();
	DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

	int middle = GetScreenWidth()/2;
	//draw menu
	

	float textSize = measureText("Finished", GetScreenWidth() * 0.15);
	drawText("Finished", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.05, GetScreenWidth() * 0.15, WHITE);

	char * tmpString = malloc(50);
	sprintf(tmpString, "%s", _highScore<_score ? "New highscore!" : "");
	textSize = measureText(tmpString, GetScreenWidth() * 0.15);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.2, GetScreenWidth() * 0.1, WHITE);


	//draw score
	sprintf(tmpString, "Score: %i Combo %i", _score, _highestCombo);
	textSize = measureText(tmpString, GetScreenWidth() * 0.1);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.5, GetScreenWidth() * 0.07, LIGHTGRAY);

	//draw highscore
	sprintf(tmpString, "Highscore: %i Combo :%i", _highScore, _highScoreCombo);
	textSize = measureText(tmpString, GetScreenWidth() * 0.05);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.6, GetScreenWidth() * 0.05, LIGHTGRAY);
	free(tmpString);

	if(interactableButton("Retry", 0.05,middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.7, GetScreenWidth()*0.3,GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//retrying map
		printf("retrying map! \n");
		
		_pGameplayFunction = &fCountDown;
		_musicHead = 0;
		_transition = 0.1;
	}
	if(interactableButton("Main Menu", 0.05,middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.85, GetScreenWidth()*0.3, GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fPlaying;
	}
	drawVignette();
	drawCursor();
}

void fEditor ()
{
	static bool isPlaying = false;
	_musicPlaying = isPlaying;
	if(isPlaying) {
		_musicHead += GetFrameTime();
		if (endOfMusic())
		{
			_musicPlaying = false;
		}
	}else
	{
		setMusicFrameCount();
		if(IsKeyDown(KEY_RIGHT) || GetMouseWheelMove() > 0) _musicHead+= GetFrameTime()*_scrollSpeed;
		if(IsKeyDown(KEY_LEFT) || GetMouseWheelMove() < 0) _musicHead-= GetFrameTime()*_scrollSpeed;
		if(IsKeyPressed(KEY_UP) || (GetMouseWheelMove() > 0 && IsKeyDown(KEY_LEFT_CONTROL))) _scrollSpeed *= 1.2;
		if(IsKeyPressed(KEY_DOWN) || (GetMouseWheelMove() < 0 && IsKeyDown(KEY_LEFT_CONTROL))) _scrollSpeed /= 1.2;
		if(_scrollSpeed == 0) _scrollSpeed = 0.01;
		//todo new raylib version renable this
		if(IsMouseButtonDown(2)) 
		{
			_musicHead -= GetMouseDelta().x/GetScreenWidth()*_scrollSpeed;
		}
	}
	if(getMusicHead() < 0)
		_musicHead = 0;

	if(IsKeyPressed(KEY_ESCAPE)) {
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fEditor;
		return;
	}
	
	if(getMusicHead() > getMusicDuration())
		_musicHead = getMusicDuration();

	ClearBackground(BLACK);
	
	drawBackground();

	//draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	dNotes();
	
	if(_isKeyPressed)
	{
		float closestTime = 55;
		int closestIndex = 0;
		for(int i = 0; i < _amountNotes; i++)
		{
			if(closestTime > fabs(_pNotes[i] - getMusicHead()))
			{
				closestTime = fabs(_pNotes[i] - getMusicHead());
				closestIndex = i;
			}
		}
		if (IsKeyPressed(KEY_X) && closestTime < _maxMargin)
		{
			removeNote(closestIndex);
		}
		if(IsKeyPressed(KEY_Z) && closestTime > 0.03f)
		{
			newNote(getMusicHead());
		}

		if(IsKeyPressed(KEY_C) && isPlaying)
		{
			//todo maybe not 4 subbeats?
			float secondsPerBeat = getMusicDuration() / getBeatsCount()/4;
			_musicHead = roundf(getMusicHead()/secondsPerBeat)*secondsPerBeat;
		}

		if(IsKeyPressed(KEY_SPACE))
		{
			isPlaying = !isPlaying;
		}
	}

	drawMusicGraph(0.7);
	drawVignette();
	
	char bpm[10] = {0};
	if(_map->bpm != 0)
		sprintf(bpm, "%i", _map->bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x=GetScreenWidth()*0.8, .y=GetScreenHeight()*0.1, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(bpmBox, bpm, &bpmBoxSelected);
	_map->bpm=atoi(bpm);
	_map->bpm = fmin(fmax(_map->bpm, 0), 300);

	char offset[10] = {0};
	if(_map->offset != 0)
		sprintf(offset, "%i", _map->offset);
	static bool offsetBoxSelected = false;
	Rectangle offsetBox = (Rectangle){.x=GetScreenWidth()*0.8, .y=GetScreenHeight()*0.18, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(offsetBox, offset, &offsetBoxSelected);
	_map->offset = atoi(offset);
	_map->offset = fmin(fmax(_map->offset, 0), 5000);
		

	
	drawBars();
	drawProgressBarI(true);
	drawCursor();
}

void fRecording ()
{
	if(IsKeyPressed(KEY_ESCAPE)) {
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fRecording;
		return;
	}
	_musicHead += GetFrameTime();
	fixMusicTime();

		ClearBackground(BLACK);
		drawBackground();

		if(_isKeyPressed && getMusicHead!=0)
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
	if(endOfMusic())
	{
		saveFile(_amountNotes);
		unloadMap();
		gotoMainMenu(true);
	}
}

bool isAnyKeyDown() {
	return GetKeyPressed() || 
		IsMouseButtonPressed(0) || 
		IsGamepadButtonPressed(0, GetGamepadButtonPressed()) ||
		IsGamepadButtonPressed(1, GetGamepadButtonPressed()) ||
		IsGamepadButtonPressed(2, GetGamepadButtonPressed()) ||
		IsGamepadButtonPressed(3, GetGamepadButtonPressed());
}

#define RippleAmount 10
#define feedback(newFeedback, size) feedbackSayings[feedbackIndex] = newFeedback; feedbackSize[feedbackIndex] = size; feedbackIndex++; if(feedbackIndex > 4) feedbackIndex = 0;
#define addRipple(newRipple) rippleEffect[rippleEffectIndex] = 0; rippleEffectStrength[rippleEffectIndex] = newRipple; rippleEffectIndex = (rippleEffectIndex+1)%RippleAmount;
void fPlaying ()
{
	static char *feedbackSayings [5];
	static float feedbackSize [5];
	static int feedbackIndex = 0;
	static float rippleEffect[RippleAmount] = {0};
	static float rippleEffectStrength[RippleAmount] = {0};
	static int rippleEffectIndex = 0;
	static float smoothHealth = 50;
	_musicHead += GetFrameTime();
	_musicPlaying = true;
	fixMusicTime();



	if(endOfMusic())
	{
		stopMusic();
		readScore(_map, &_highScore, &_highScoreCombo);
		if(_highScore < _score)
			saveScore();
		_pGameplayFunction = &fEndScreen;
		playAudioEffect(_pFinishSE, _finishSE_Size);
		_transition = 0.1;
		return;
	}
	if(IsKeyPressed(KEY_ESCAPE))
	{
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fPlaying;
	}

	ClearBackground(BLACK);
	
	drawBackground();


	//draw ripples
	for(int i = 0; i < RippleAmount; i++)
	{
		if(rippleEffectStrength[i] == 0)
			continue;
		rippleEffect[i] += GetFrameTime()*1200*rippleEffectStrength[i];
		rippleEffectStrength[i] = fmax(rippleEffectStrength[i] - GetFrameTime()*5, 0);
		float size = rippleEffect[i];
		DrawRing((Vector2){.x=GetScreenWidth()/2, .y=GetScreenHeight()*0.42}, size, size*0.7, 0, 360, 50, ColorAlpha(WHITE, rippleEffectStrength[i]));
	}

	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	
	dNotes();


	
	if(_combo > _highestCombo) _highestCombo = _combo;

	//draw feedback (300! 200! miss!)
	for(int i = 0, j = feedbackIndex-1; i < 5; i++, j--)
	{
		if(j < 0) j = 4;
		drawText(feedbackSayings[j], GetScreenWidth() * 0.35, GetScreenHeight() * (0.6 + i * 0.1), GetScreenWidth() * 0.05*feedbackSize[j], (Color){.r=255,.g=255,.b=255,.a=noLessThanZero(150 - i * 40)});
	}

	if(_noteIndex < _amountNotes && getMusicHead() - _maxMargin > _pNotes[_noteIndex])
	{
		//passed note
		_noteIndex++;
		feedback("miss!", 1.3-_health/100);
		_health -= _missPenalty;
		_combo = 0;
		playAudioEffect(_pMissSE, _missSE_Size);
	}

	if(_isKeyPressed && _noteIndex < _amountNotes)
	{
		float closestTime = 55;
		int closestIndex = 0;
		for(int i = _noteIndex; i <= _noteIndex + 1 && i < _amountNotes; i++)
		{
			if(closestTime > fabs(_pNotes[i] - getMusicHead()))
			{
				closestTime = fabs(_pNotes[i] - getMusicHead());
				closestIndex = i;
			}
		}
		if(closestTime < _maxMargin)
		{
			while(_noteIndex < closestIndex)
			{
				_noteIndex++;
				feedback("miss!", 1.3-_health/100);
				_combo = 0;
				_health -= _missPenalty;
			}
			int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / _maxMargin));
			_health += healthAdded;
			int scoreAdded = noLessThanZero(300 - closestTime * (300 / _maxMargin));
			if(scoreAdded > 200) {
				feedback("300!", 1.2);
				addRipple(1);
			}else if (scoreAdded > 100) {
				feedback("200!", 1);
				addRipple(0.6);
			} else {
				feedback("100!", 0.8);
				addRipple(0.3);
			}
			_score += scoreAdded * (1+_combo/100);
			_noteIndex++;
			_combo++;
			playAudioEffect(_pHitSE, _hitSE_Size);
		}else
		{
			printf("missed note\n");
			feedback("miss!", 1.3-_health/100);
			_combo = 0;
			_health -= _missPenalty;
			playAudioEffect(_pMissHitSE, _missHitSE_Size);
		}
		ClearBackground(BLACK);
	}

	if(_health > 100)
		_health = 100;
	if(_health < 0)
		_health = 0;

	if(_health < 25)
		_fade = ColorAlpha(RED, 1-_health/25);
	else _fade = ColorAlpha(RED, 0);

	smoothHealth += (_health-smoothHealth)*GetFrameTime()*3;

	if(feedbackIndex >= 5)
		feedbackIndex = 0;

	float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale,
		.y=GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - smoothHealth / 250)}, 0, heartScale,WHITE);

	if(_health <= 0)
	{
		printf("goto fail\n");
		//goto fFail
		stopMusic();
		_pGameplayFunction = &fFail;
		playAudioEffect(_pFailSE, _failSE_Size);
		_transition = 0.1;
	}
	drawVignette();

	//draw score
	char * tmpString = malloc(20);
	sprintf(tmpString, "score: %i", _score);
	drawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);

	//draw combo
	sprintf(tmpString, "combo: %i", _combo);
	drawText(tmpString, GetScreenWidth() * 0.70, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
	free(tmpString);
	drawProgressBar();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), _fade);
}

void fFail ()
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	if(!_noBackground)
	{
		float scale = (float)GetScreenWidth() / (float)_background.width;
		DrawTextureEx(_background, (Vector2){.x=0, .y=(GetScreenHeight() - _background.height * scale)/2}, 0, scale,WHITE);
	}else{
		DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
	}
	DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

	int middle = GetScreenWidth()/2;
	//draw menu
	
	float textSize = measureText("You Failed", GetScreenWidth() * 0.15);
	drawText("You Failed", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.2, GetScreenWidth() * 0.15, WHITE);

	//draw score
	char * tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	textSize = measureText(tmpString, GetScreenWidth() * 0.1);
	drawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.5, GetScreenWidth() * 0.1, LIGHTGRAY);
	free(tmpString);

	if(interactableButton("Retry", 0.05, middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.7, GetScreenWidth()*0.3, GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//retrying map
		printf("retrying map! \n");
		_pGameplayFunction = &fCountDown;
		_musicHead = 0;
		_transition = 0.7;
	}
	if(interactableButton("Exit", 0.05, middle - GetScreenWidth()*0.15, GetScreenHeight() * 0.85, GetScreenWidth()*0.3, GetScreenHeight()*0.1))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//retrying map
		printf("going to main Menu! \n");
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fPlaying;
	}
	drawVignette();
	drawCursor();
}

void fMapSelect()
{
	static int amount = 0;
	static char ** files = 0;
	static int highScores[100];
	static int combos[100];
	checkFileDropped();
	if(_mapRefresh)
	{
		files = GetDirectoryFiles("maps/", &amount);
		int mapIndex = 0;
		for(int i = 0; i < amount; i++)
		{
			if(files[i][0] == '.')
				continue;
			_pMaps[mapIndex] = loadMapInfo(&(files[i][0]));
			if(_pMaps[mapIndex].name != 0)
			{
				readScore(&_pMaps[mapIndex], &(highScores[mapIndex]), &(combos[mapIndex]));
				mapIndex++;
			}
		}
		amount = mapIndex;
		_mapRefresh = false;
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth()/2;

	static float menuScroll = 0;
	menuScroll += GetMouseWheelMove()*.04;
	if(IsMouseButtonDown(0)) { //scroll by dragging 
		menuScroll += GetMouseDelta().y / GetScreenHeight();
	}
	const float scrollSpeed = .03;
	if(IsKeyDown(KEY_UP)) {
		menuScroll += scrollSpeed;
	}
	if(IsKeyDown(KEY_DOWN)) {
		menuScroll -= scrollSpeed;
	}
	menuScroll = clamp(menuScroll, -.5*floor(amount/2), 0);

	//draw map button
	for(int i = 0; i < amount; i++)
	{
		//draw main menu
		int x = GetScreenWidth()*0.05;
		if(i % 2 == 1)
			x = GetScreenWidth()*0.55;
		Rectangle mapButton = (Rectangle){.x=x, .y=menuScroll*GetScreenHeight()+GetScreenHeight() * (0.3+0.5*floor(i/2)), .width=GetScreenWidth()*0.4,.height=GetScreenHeight()*0.4};
		drawMapThumbnail(mapButton,&_pMaps[i], highScores[i], combos[i]);

		if(IsMouseButtonReleased(0) && mouseInRect(mapButton))
		{
			playAudioEffect(_pButtonSE, _buttonSE_Size);
			_map = &_pMaps[i];
			//switching to playing map
			
			loadMap();
			_pGameplayFunction = _pNextGameplayFunction;
			if(_pNextGameplayFunction == &fRecording)
			{
				free(_pNotes);
				_pNotes = calloc(sizeof(float), 50);
			}
			setMusicStart();
			_musicHead = 0;
			if(_pNextGameplayFunction == &fPlaying || _pNextGameplayFunction == &fRecording)
				_pGameplayFunction = &fCountDown;
			
			if(_pGameplayFunction == &fEditor)
				startMusic();
			printf("selected map!\n");
			_transition = 0.1;
		}
	}
	drawVignette();
	

	if(interactableButton("Back", 0.03, GetScreenWidth()*0.05, GetScreenHeight()*0.05, GetScreenWidth()*0.1, GetScreenHeight()*0.05))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		_pGameplayFunction=&fMainMenu;
		_transition = 0.1;
		return;
	}

	

	if(interactableButton("New Map", 0.03, GetScreenWidth()*0.70, GetScreenHeight()*0.9, GetScreenWidth()*0.15, GetScreenHeight()*0.07))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		_pGameplayFunction=&fNewMap;
		_transition = 0.1;
		return;
	}

	drawCursor();
}

void fExport()
{
	_pGameplayFunction = &fMainMenu;
	makeMapZip(_map);
	char str [300];
	strcpy(str, GetWorkingDirectory());
	strcat(str, "/");
	strcat(str, _map->name);
	strcat(str, ".zip");
	SetClipboardText(str);
	resetBackGround();
}

void fNewMap()
{
	static Map newMap = {0};
	static void * pMusic = 0;
	static char pMusicExt [50];
	static int pMusicSize = 0;
	static void * pImage = 0;
	static int imageSize = 0;

	if(newMap.name == 0)
	{
		newMap.name = malloc(100);
		strcpy(newMap.name, "name");
		newMap.creator = malloc(100);
		newMap.creator[0] = '\0';
		strcpy(newMap.creator, "creator");
		newMap.folder = malloc(100);
		newMap.folder[0] = '\0';
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth()/2;

	if(interactableButton("Back", 0.03,GetScreenWidth()*0.05, GetScreenHeight()*0.05, GetScreenWidth()*0.1, GetScreenHeight()*0.05))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		_pGameplayFunction=&fMapSelect;
		_transition = 0.1;
		return;
	}
	
	if(interactableButton("Finish", 0.02, GetScreenWidth()*0.85, GetScreenHeight()*0.85, GetScreenWidth()*0.1, GetScreenHeight()*0.05))
	{
		if(pMusic == 0)
			return;
		if(pImage == 0)
			return;
		makeMap(&newMap);
		char str [100];
		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/song");
		strcat(str, pMusicExt);
		FILE * file = fopen(str, "w");
		fwrite(pMusic, pMusicSize, 1, file);
		fclose(file);

		newMap.musicFile = malloc(100);
		strcpy(newMap.musicFile, str);

		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/image.png");
		file = fopen(str, "w");
		fwrite(pImage, imageSize, 1, file);
		fclose(file);
		if(newMap.bpm == 0)
			newMap.bpm = 1;
		_pGameplayFunction=&fRecording;
		_transition = 0.1;
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		newMap.folder = malloc(100);
		strcpy(newMap.folder, newMap.name);
		_map = &newMap;
		saveFile(0);
		loadMap();
		_noBackground = 0;
		return;
	}

	//text boxes
	static bool nameBoxSelected = false;
	Rectangle nameBox = (Rectangle){.x=middle, .y=GetScreenHeight()*0.5, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(nameBox, newMap.name, &nameBoxSelected);

	static bool creatorBoxSelected = false;
	Rectangle creatorBox = (Rectangle){.x=middle, .y=GetScreenHeight()*0.625, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(creatorBox, newMap.creator, &creatorBoxSelected);

	char str[100] = {'\0'};
	if(newMap.bpm != 0)
		sprintf(str, "%i", newMap.bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x=middle, .y=GetScreenHeight()*0.875, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	textBox(bpmBox, str, &bpmBoxSelected);
	newMap.bpm = fmin(fmax(atoi(str), 0), 500);

	static bool difficultyBoxSelected = false;
	Rectangle difficultyBox = (Rectangle){.x=middle, .y=GetScreenHeight()*0.75, .width=GetScreenWidth()*0.2, .height=GetScreenHeight()*0.07};
	numberBox(difficultyBox, &newMap.difficulty, &difficultyBoxSelected);
	

	int textSize = measureText("Drop in .png, .wav or .mp3", GetScreenWidth() * 0.04);
	drawText("Drop in .png, .wav or .mp3", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.2, GetScreenWidth() * 0.04, WHITE);

	textSize = measureText("missing music file", GetScreenWidth() * 0.03);
	if(pMusic == 0)
		drawText("missing music file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight()*0.6, GetScreenWidth() * 0.03, WHITE);
	else 
		drawText("got music file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight()*0.6, GetScreenWidth() * 0.03, WHITE);

	textSize = measureText("missing image file", GetScreenWidth() * 0.03);
	if(pImage == 0)
		drawText("missing image file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight()*0.7, GetScreenWidth() * 0.03, WHITE);
	else
		drawText("got image file", GetScreenWidth() * 0.2 - textSize / 2, GetScreenHeight()*0.7, GetScreenWidth() * 0.03, WHITE);



	drawCursor();


	//file dropping
	if(IsFileDropped() || IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
	{
		printf("yoo new file dropped boys\n");
		int amount = 0;
		char ** files;
		bool keyOrDrop = true;
		if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			//copy paste
			const char * str = GetClipboardText();

			int file = 0;
			char part [100] = {0};
			int partIndex = 0;
			//todo no hardcode, bad hardcode!
			files = malloc(100);
			for(int i = 0; str[i] != '\0'; i++)
			{
				part[partIndex++]=str[i];
				if(str[i+1] == '\n' || str[i+1] == '\0')
				{
					i++;
					part[partIndex+1] = '\0';
					printf("file %i: %s\n", file, part);
					if(FileExists(part))
					{
						printf("\tfile exists\n");
						files[file] = malloc(partIndex);
						strcpy(files[file], part);
						file++;
					}
					partIndex=0;
				}
			}
			amount = file;
			keyOrDrop = false;
		}
		else files = GetDroppedFiles(&amount);
		for(int i = 0; i < amount; i++)
		{
			const char * ext = GetFileExtension(files[i]);
			printf("%s\n", ext);
			if(strcmp(ext, ".png") == 0)
			{
				if(newMap.image.id != 0)
					UnloadTexture(newMap.image);
				newMap.image = LoadTexture(files[i]);

				if(pImage != 0)
					free(pImage);
				FILE * file = fopen(files[i], "r");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pImage = malloc(size);
				fread(pImage, size, 1, file);
				fclose(file);
				imageSize = size;
			}

			if(strcmp(ext, ".mp3") == 0)
			{
				if(pMusic != 0)
					free(pMusic);
				FILE * file = fopen(files[i], "r");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pMusic = malloc(size);
				fread(pMusic, size, 1, file);
				fclose(file);
				strcpy(pMusicExt, ext);
				pMusicSize = size;
			}

			if(strcmp(ext, ".wav") == 0)
			{
				if(pMusic != 0)
					free(pMusic);
				FILE * file = fopen(files[i], "r");
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
		if(!keyOrDrop)
		{
			for(int i = 0; i < amount; i++)
				free(files[i]);
			free(files);
		}
		else ClearDroppedFiles();
	}
}