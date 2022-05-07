#ifndef MAINC
#include "gameplay.h"
#include "files.h"
#include "drawing.h"
#include "audio.h"
#endif

#include "include/raylib.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern Texture2D _noteTex, _background, _heartTex, _healthBarTex;
extern Color _fade;
extern bool _musicPlaying;
extern float _musicHead, _transition;
extern void * _pHitSE, *_pMissHitSE, *_pMissSE, *_pButtonSE;
extern int _hitSE_Size, _missHitSE_Size, _missSE_Size, _buttonSE_Size;


extern Map _pMaps[100];

float _scrollSpeed = 0.6;
int _noteIndex = 0, _amountNotes =0 ;
bool _noBackground = false;
float _health = 50;
int _score= 0;
int _bpm = 100;
float _maxMargin = 0.1;
int _hitPoints = 5;
int _missPenalty = 10;
char *_pMap = "testMap";
float _fadeOut = 0;

//Timestamp of all the notes
float * _pNotes;
void (*_pNextGameplayFunction)();
void (*_pGameplayFunction)();

void gotoMainMenu()
{
	stopMusic();
	loadMusic("menuMusic.mp3");
	_musicPlaying = true;
	randomMusicPoint();
	_pGameplayFunction = &fMainMenu;
	resetBackGround();
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

	if(GetKeyPressed() && _musicHead!=0)
	{
		newNote(_musicHead);
		ClearBackground(BLACK);
	}
	dNotes();
	drawVignette();
	drawProgressBar();
	DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

	float middle = GetScreenWidth()/2;
	Rectangle continueButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.3, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(continueButton,"continue", 0.05);

	Rectangle exitButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.5, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(exitButton,"exit", 0.05);

	if(IsMouseButtonReleased(0))
	{
		if(mouseInRect(continueButton))
		{
			playAudioEffect(_pButtonSE, _buttonSE_Size);
			if(_pGameplayFunction == &fPlaying)
				_pGameplayFunction = &fCountDown;
			else
				_pGameplayFunction = _pNextGameplayFunction;
		}
		if(mouseInRect(exitButton))
		{
			playAudioEffect(_pButtonSE, _buttonSE_Size);
			unloadMap();
			gotoMainMenu();
		}
	}
	drawCursor();
}

void fCountDown ()
{
	_musicPlaying = false;
	static float countDown  = 0;
	if(countDown == 0) countDown = GetTime() + 3;
	if(countDown - GetTime() +GetFrameTime() <= 0)
	{
		countDown = 0;
		//switching to playing map
		printf("switching to playing map! \n");
		_pGameplayFunction = _pNextGameplayFunction;
		startMusic();
		
		_health = 50;
		_score = 0;
		_noteIndex =1;
		_musicHead = 0;
		return;
	}
	if(_musicHead <= 0) _musicHead = GetTime()-countDown;

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
	DrawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
	
	drawCursor();
	
	float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale,
		.y=GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale,WHITE);
	DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

	sprintf(tmpString, "%i", (int)(countDown - GetTime() + 1));
	float textSize = MeasureText(tmpString, GetScreenWidth() * 0.3);
	DrawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.3, GetScreenWidth() * 0.3, WHITE);
	free(tmpString);
	drawVignette();
}

void fMainMenu()
{
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth()/2;
	//draw main menu
	Rectangle playButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.3, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(playButton,"play", 0.05);

	Rectangle editorButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.5, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(editorButton,"Editor", 0.05);

	Rectangle recordingButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.7, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(recordingButton,"Record", 0.05);
	

	if(IsMouseButtonReleased(0) && mouseInRect(playButton))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//switching to playing map
		printf("switching to playing map!\n");
		_pNextGameplayFunction = &fPlaying;
		_pGameplayFunction = &fMapSelect;
		_transition = 0.1;
	}

	if(IsMouseButtonReleased(0) && mouseInRect(editorButton))
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

	if(IsMouseButtonReleased(0) && mouseInRect(recordingButton))
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

	//gigantic ass title 
	char * title = "One Button Rhythm";
	float tSize = GetScreenWidth()*0.1;
	int size = MeasureText(title, tSize);
	//dropshadow
	DrawText(title, middle-size/2+GetScreenWidth()*0.004, GetScreenHeight()*0.107, tSize, DARKGRAY);
	//real title
	DrawText(title, middle-size/2, GetScreenHeight()*0.1, tSize, WHITE);

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
	Rectangle playButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.7, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(playButton,"retry", 0.05);

	Rectangle MMButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.85, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(MMButton,"main menu", 0.05);

	float textSize = MeasureText("Finished", GetScreenWidth() * 0.15);
	DrawText("Finished", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.2, GetScreenWidth() * 0.15, WHITE);

	//draw score
	char * tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	textSize = MeasureText(tmpString, GetScreenWidth() * 0.1);
	DrawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.5, GetScreenWidth() * 0.1, LIGHTGRAY);
	free(tmpString);

	if(IsMouseButtonReleased(0) && mouseInRect(playButton))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//retrying map
		printf("retrying map! \n");
		
		_pGameplayFunction = &fCountDown;
		_transition = 0.1;
	}
	if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		unloadMap();
		gotoMainMenu();
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
	}else
	{
		setMusicFrameCount();
		if(IsKeyDown(KEY_RIGHT) || GetMouseWheelMove() > 0) _musicHead+= GetFrameTime()*_scrollSpeed;
		if(IsKeyDown(KEY_LEFT) || GetMouseWheelMove() < 0) _musicHead-= GetFrameTime()*_scrollSpeed;
		if(IsKeyPressed(KEY_UP)) _scrollSpeed *= 1.2;
		if(IsKeyPressed(KEY_DOWN)) _scrollSpeed /= 1.2;
		if(_scrollSpeed == 0) _scrollSpeed = 0.01;
	}
	if(_musicHead < 0)
		_musicHead = 0;

	if(IsKeyPressed(KEY_ESCAPE)) {
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fEditor;
		return;
	}

	if(_musicHead > getMusicDuration())
		_musicHead = getMusicDuration();
	

	ClearBackground(BLACK);
	
	drawBackground();

	//draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	dNotes();
	
	if(GetKeyPressed())
	{
		float closestTime = 55;
		int closestIndex = 0;
		for(int i = 0; i < _amountNotes; i++)
		{
			if(closestTime > fabs(_pNotes[i] - _musicHead))
			{
				closestTime = fabs(_pNotes[i] - _musicHead);
				closestIndex = i;
			}
		}
		if (IsKeyPressed(KEY_X) && closestTime < _maxMargin)
		{
			removeNote(closestIndex);
		}
		if(IsKeyPressed(KEY_Z) && closestTime > 0.03f)
		{
			newNote(_musicHead);
		}

		if(IsKeyPressed(KEY_C))
		{
			//todo maybe not 4 subbeats?
			float secondsPerBeat = getMusicDuration() / getBeatsCount()/4;
			_musicHead = roundf(_musicHead/secondsPerBeat)*secondsPerBeat;
		}

		if(IsKeyPressed(KEY_SPACE))
		{
			isPlaying = !isPlaying;
		}
	}
	drawMusicGraph(0.7);
	drawVignette();
	drawBars();
	drawProgressBarI(true);
	drawCursor();

	if(endOfMusic())
	{
		loadMap(1);
		saveFile(_amountNotes);
		gotoMainMenu();
	}
}

void fRecording ()
{
	if(IsKeyPressed(KEY_ESCAPE)) {
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fEditor;
		return;
	}
	_musicHead += GetFrameTime();
	fixMusicTime();

		ClearBackground(BLACK);
		drawBackground();

		if(GetKeyPressed() && _musicHead!=0)
		{
			printf("keyPressed! \n");
			
			newNote(_musicHead);
			printf("music Time: %.2f\n", _musicHead);
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
		gotoMainMenu();
	}
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

		saveScore();
		_pGameplayFunction = &fEndScreen;
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
		printf("drawing ripple\n");
		rippleEffect[i] += GetFrameTime()*1200*rippleEffectStrength[i];
		rippleEffectStrength[i] = fmax(rippleEffectStrength[i] - GetFrameTime()*5, 0);
		float size = rippleEffect[i];
		DrawRing((Vector2){.x=GetScreenWidth()/2, .y=GetScreenHeight()*0.42}, size, size*0.7, 0, 360, 50, ColorAlpha(WHITE, rippleEffectStrength[i]));
	}

	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	
	dNotes();


	//draw score
	char * tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	DrawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
	free(tmpString);


	//draw feedback (300! 200! miss!)
	for(int i = 0, j = feedbackIndex-1; i < 5; i++, j--)
	{
		if(j < 0) j = 4;
		DrawText(feedbackSayings[j], GetScreenWidth() * 0.35, GetScreenHeight() * (0.6 + i * 0.1), GetScreenWidth() * 0.05*feedbackSize[j], (Color){.r=255,.g=255,.b=255,.a=noLessThanZero(150 - i * 40)});
	}

	if(_noteIndex < _amountNotes && _musicHead - _maxMargin > _pNotes[_noteIndex])
	{
		//passed note
		_noteIndex++;
		feedback("miss!", 1.3-_health/100);
		_health -= _missPenalty;
		playAudioEffect(_pMissSE, _missSE_Size);
	}

	if(GetKeyPressed() && _noteIndex < _amountNotes)
	{
		float closestTime = 55;
		int closestIndex = 0;
		for(int i = _noteIndex; i <= _noteIndex + 1 && i < _amountNotes; i++)
		{
			if(closestTime > fabs(_pNotes[i] - _musicHead))
			{
				closestTime = fabs(_pNotes[i] - _musicHead);
				closestIndex = i;
			}
		}
		if(closestTime < _maxMargin)
		{
			while(_noteIndex < closestIndex)
			{
				_noteIndex++;
				feedback("miss!", 1.3-_health/100);
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
			_score += scoreAdded;
			_noteIndex++;
			playAudioEffect(_pHitSE, _hitSE_Size);
		}else
		{
			printf("missed note\n");
			feedback("miss!", 1.3-_health/100);
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
		_transition = 0.1;
	}
	drawVignette();
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
	Rectangle playButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.7, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(playButton,"retry", 0.05);

	Rectangle MMButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.85, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
	drawButton(MMButton,"main menu", 0.05);
	
	

	float textSize = MeasureText("You Failed", GetScreenWidth() * 0.15);
	DrawText("You Failed", GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.2, GetScreenWidth() * 0.15, WHITE);

	//draw score
	char * tmpString = malloc(9);
	sprintf(tmpString, "%i", _score);
	textSize = MeasureText(tmpString, GetScreenWidth() * 0.1);
	DrawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.5, GetScreenWidth() * 0.1, LIGHTGRAY);
	free(tmpString);

	if(IsMouseButtonReleased(0) && mouseInRect(playButton))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//retrying map
		printf("retrying map! \n");
		_pGameplayFunction = &fCountDown;
		_transition = 0.7;
	}
	if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		//retrying map
		printf("going to main Menu! \n");
		unloadMap();
		gotoMainMenu();
	}
	drawVignette();
	drawCursor();
}

void fMapSelect()
{
	static int amount = 0;
	static char ** files = 0;
	if(files == 0)
	{
		files = GetDirectoryFiles("maps/", &amount);
		// _pMaps = calloc(sizeof(_pMaps), amount);
		int mapIndex = 0;
		for(int i = 0; i < amount; i++)
		{
			if(files[i][0] == '.')
				continue;
			_pMaps[mapIndex] = loadMapInfo(&(files[i][0]));
			mapIndex++;
		}
		amount = mapIndex;
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);

	int middle = GetScreenWidth()/2;

	Rectangle backButton = (Rectangle){.x=GetScreenWidth()*0.05, .y=GetScreenHeight()*0.05, .width=GetScreenWidth()*0.1, .height=GetScreenHeight()*0.05};
	drawButton(backButton, "back", 0.02);

	if(mouseInRect(backButton) && IsMouseButtonDown(0))
	{
		playAudioEffect(_pButtonSE, _buttonSE_Size);
		_pGameplayFunction=&fMainMenu;
		_transition = 0.1;
		return;
	}

	//draw map button
	for(int i = 0; i < amount; i++)
	{
		//draw main menu
		int x = GetScreenWidth()*0.05;
		if(i % 2 == 1)
			x = GetScreenWidth()*0.55;
		Rectangle mapButton = (Rectangle){.x=x, .y=GetScreenHeight() * (0.3+0.4*floor(i/2)), .width=GetScreenWidth()*0.4,.height=GetScreenHeight()*0.3};
		drawMapThumbnail(mapButton,&_pMaps[i]);

		if(IsMouseButtonReleased(0) && mouseInRect(mapButton))
		{
			playAudioEffect(_pButtonSE, _buttonSE_Size);
			_pMap = malloc(100);
			strcpy(_pMap, _pMaps[i].folder);
			printf("map %s");
			//switching to playing map
			if(_pNextGameplayFunction != &fRecording)
				loadMap(0);
			else
				loadMap(1);
			_pGameplayFunction = _pNextGameplayFunction;
			
			setMusicStart();
			
			if(_pNextGameplayFunction == &fPlaying || _pNextGameplayFunction == &fRecording)
				_pGameplayFunction = &fCountDown;
			
			if(_pGameplayFunction == &fEditor)
				startMusic();
			printf("selected map!\n");
			_transition = 0.1;
			
		}
	}
	drawCursor();
}