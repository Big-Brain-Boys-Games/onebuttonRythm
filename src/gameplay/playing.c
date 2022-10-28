#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../deps/raylib/src/raylib.h"


#define EXTERN_AUDIO
#define EXTERN_DRAWING
#define EXTERN_GAMEPLAY
#define EXTERN_MAIN

#include "playing.h"

#include "../drawing.h"
#include "../audio.h"
#include "../main.h"

#include "editor.h"
#include "menus.h"
#include "recording.h"
#include "gameplay.h"


float _health = 50;
int _missPenalty = 5;
int _hitPoints = 5;



void fCountDown(bool reset)
{
	
	static float countDown = 0;
	static bool contin = false;
	if (reset)
	{
		if(_musicHead <= 0)
		{
			contin = false;	
		}else
		{
			contin = true;
		}
		countDown = 3 + GetTime();
		printf("fCountDown reset (%i)\n", contin);
		return;
	}

	_musicLoops = false;
	_musicPlaying = false;

	if( _musicHead <= 0 )
		_musicHead = GetTime() - countDown;

	


	if (GetTime() + GetFrameTime() >= countDown)
	{
		countDown = 0;
		_pGameplayFunction = _pNextGameplayFunction;
		if (!contin)
		{
			// switching to playing map
			printf("switch to Playing\n");
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
	ClearBackground(BLACK);
	drawBackground();

	// draw notes
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() / 2;

	dNotes();

	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

	// DrawRectangle(middle - width / 2, 0, width, GetScreenHeight(), (Color){.r = 255, .g = 255, .b = 255, .a = 255 / 2});

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




#define HITPOINTAMOUNT 10

#define RippleAmount 10
#define feedback(newFeedback, size)               \
	strcpy(feedbackSayings[feedbackIndex],newFeedback); \
	feedbackSize[feedbackIndex] = size;           \
	feedbackIndex++;                              \
	if (feedbackIndex > 4)                        \
		feedbackIndex = 0;
#define addRipple(newRipple)                             \
	rippleEffect[rippleEffectIndex] = 0;                 \
	rippleEffectStrength[rippleEffectIndex] = newRipple; \
	rippleEffectIndex = (rippleEffectIndex + 1) % RippleAmount;
void fPlaying(bool reset)
{
	static char feedbackSayings[5][50] = {0};
	static float feedbackSize[5] = {0};
	static int feedbackIndex = 0;
	static float rippleEffect[RippleAmount] = {0};
	static float rippleEffectStrength[RippleAmount] = {0};
	static int rippleEffectIndex = 0;
	static float smoothHealth = 50;

	static float hitPointTimings [HITPOINTAMOUNT] = {0}; //how off they are
	static float hitPointsTimes [HITPOINTAMOUNT] = {0}; //when they're hit
	static int hitPointScores [HITPOINTAMOUNT] = {0}; //score for hit
	static float hitPointSize [HITPOINTAMOUNT] = {0}; //score for hit
	static int hitPointsIndex = 0;

	if(reset)
	{
		loadMap();

		//reset variables
		for(int i = 0; i < 5; i++)
		{
			feedbackSayings[i][0] = '\0';
			feedbackSize[i] = 0;
		}

		smoothHealth = 50;
		feedbackIndex = 0;
		rippleEffectIndex = 0;

		for(int i = 0; i < RippleAmount; i++)
		{
			rippleEffect[i] = 0;
			rippleEffectStrength[i] = 0;
		}

		for(int i = 0; i < HITPOINTAMOUNT; i++)
		{
			hitPointTimings[i] = 0;
			hitPointsTimes[i] = -99;
		}
		return;
	}


	_musicHead += GetFrameTime() * _musicSpeed;
	_musicPlaying = true;
	fixMusicTime();

    float margin = _maxMargin * getMarginMod();


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
		playAudioEffect(_finishSe);
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

	//draw hitpoints
	for (int i = 0; i < HITPOINTAMOUNT; i++)
	{
		float alpha = 1-(_musicHead-hitPointsTimes[i] + 0.000001);
		alpha = fmin(fmax(0, alpha), 1);
		// alpha *= 0.5;

		Color hpColor = WHITE;
		if(hitPointScores[i] > 200)
		{
			hpColor = GREEN;
		}else if (hitPointScores[i] > 100)
		{
			hpColor = YELLOW;
		}else
			hpColor = RED;

		Vector3 hsv = ColorToHSV(hpColor);
		hpColor = ColorFromHSV(hsv.x, 0.4, hsv.z);

		DrawCircle( musicTimeToScreen(_musicHead+hitPointTimings[i]), GetScreenHeight()*0.4, GetScreenWidth()*0.017, ColorAlpha(hpColor, alpha));
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

    if (_noteIndex < _amountNotes-1 && getMusicHead() - margin > _papNotes[_noteIndex]->time)
	{
		// passed note
		_noteIndex++;
		feedback("miss!", 1.3 - _health / 100);
		_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
		_combo = 0;
		playAudioEffect(_missSe);
	}

	if (_isKeyPressed && _noteIndex < _amountNotes)
	{
		float closestTime = 55;
		float closestNote = 9999999;
		int closestIndex = 0;
		for (int i = _noteIndex; i <= _noteIndex + 1 && i < _amountNotes; i++)
		{
			if (closestNote > _papNotes[i]->time - getMusicHead() - margin)
			{
				closestNote = _papNotes[i]->time - getMusicHead() - margin;
				closestTime = fabs(_papNotes[i]->time - getMusicHead());
				closestIndex = i;
			}
		}
		if (fabs(closestTime) < margin)
		{
			while (_noteIndex < closestIndex)
			{
				_noteIndex++;
				feedback("miss!", 1.3 - _health / 100);
				_combo = 0;
				_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
				_notesMissed++;
			}
			_averageAccuracy = ((_averageAccuracy * (_noteIndex - _notesMissed)) + ((1 / margin) * closestTime)) / (_noteIndex - _notesMissed + 1);
			// _averageAccuracy = 0.5/_amountNotes;
			int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / margin)) * _papNotes[_noteIndex]->health;
			_health += healthAdded * (1 / (getHealthMod() + 0.1));
			int scoreAdded = noLessThanZero(300 - closestTime * (300 / margin)) * getScoreMod();
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

			float noteDist = 99;

			if(closestIndex > 0)
			{
				noteDist = fabsf(_papNotes[closestIndex]->time - _papNotes[closestIndex-1]->time);
			}
			if(closestIndex < _amountNotes-1)
			{
				float tmp = fabsf(_papNotes[closestIndex]->time - _papNotes[closestIndex+1]->time);
				noteDist = fmin(tmp, noteDist);
			}

			float customScale = GetScreenWidth() * 0.04;
			float maxDistance = (float)(GetScreenWidth() / _noteTex.width) / 9 / 2;

			float middle = GetScreenWidth() / 2;
			maxDistance =  maxDistance / (middle * (1 / _scrollSpeed));

			maxDistance = (1/_scrollSpeed) * 0.075;

			if(noteDist < maxDistance)
			{
				customScale = (noteDist/maxDistance) * GetScreenWidth() * 0.05;
			}


			hitPointTimings[hitPointsIndex] = _papNotes[closestIndex]->time - _musicHead;
			hitPointsTimes[hitPointsIndex] = _musicHead;
			hitPointScores[hitPointsIndex] = scoreAdded;
			hitPointSize[hitPointsIndex] = customScale;
			hitPointsIndex = (hitPointsIndex+1)%HITPOINTAMOUNT;

			if(_papNotes[closestIndex]->custSound)
				playAudioEffect(_papNotes[closestIndex]->custSound->sound);
			else	
				playAudioEffect(_hitSe);
		}
		else
		{
			printf("missed note\n");
			feedback("miss!", 1.3 - _health / 100);
			_combo = 0;
			_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
			playAudioEffect(_missHitSe);
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
		playAudioEffect(_failSe);
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
	snprintf(tmpString, 20, "acc: %.2f", 100 * (1 - _averageAccuracy));
	drawText(tmpString, GetScreenWidth() * 0.70, GetScreenHeight() * 0.1, GetScreenWidth() * 0.04, WHITE);
	drawRank(GetScreenWidth()*0.57, GetScreenHeight()*0.03, GetScreenWidth()*0.1, GetScreenWidth()*0.1, _averageAccuracy);
	free(tmpString);
	drawProgressBar();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), _fade);
}






void fEndScreen(bool reset)
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});


	DrawRectangle(GetScreenWidth()*0.04, GetScreenHeight()*0.27, GetScreenWidth()*0.5, GetScreenHeight()*0.7, (Color){.r = 0, .g = 0, .b = 0, .a = 128});

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

	drawText("Rank", GetScreenWidth() * 0.55, GetScreenHeight() * 0.75, GetScreenWidth() * 0.05, LIGHTGRAY);
	drawRank(GetScreenWidth()*0.7, GetScreenHeight()*0.65, GetScreenWidth()*0.2, GetScreenWidth()*0.2, _averageAccuracy);

	if (interactableButton("Retry", 0.05, GetScreenWidth() * 0.15, GetScreenHeight() * 0.72, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		// retrying map
		printf("retrying map! \n");

		_pGameplayFunction = &fCountDown;
		fCountDown(true);
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




void fFail(bool reset)
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
		fCountDown(true);
		_musicHead = 0;
		_transition = 0.7;
	}
	if (interactableButton("Exit", 0.05, middle - GetScreenWidth() * 0.15, GetScreenHeight() * 0.85, GetScreenWidth() * 0.3, GetScreenHeight() * 0.1))
	{
		// exiting map
		printf("going to main Menu! \n");
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fPlaying;
	}
	drawVignette();
	drawCursor();
}