#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "../../deps/raylib/src/raylib.h"


#define EXTERN_AUDIO
#define EXTERN_DRAWING
#define EXTERN_GAMEPLAY
#define EXTERN_MAIN
#define EXTERN_MENUS

#include "playing.h"

#include "../drawing.h"
#include "../audio.h"
#include "../files.h"
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
			_score = 0;
			_combo = 0;
		}else
		{
			contin = true;
		}
		countDown = 3 + GetTime();
		printf("fCountDown reset (%i)\n", contin);

		if(_map->zoom)
				_wantedScrollSpeed = 4.2 / _map->zoom;
			
		if (!_settings.useMapZoom || !_map->zoom)
			_wantedScrollSpeed = 4.2 / _settings.zoom;
		return;
	}

	_framesOffset = _settings.offset*48; //_offset in ms

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
			if(_map->zoom)
				_wantedScrollSpeed = 4.2 / _map->zoom;
			
			if (!_settings.useMapZoom || !_map->zoom)
				_wantedScrollSpeed = 4.2 / _settings.zoom;

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
	dNotes();

	// DrawRectangle(middle - width / 2, 0, width, getHeight(), (Color){.r = 255, .g = 255, .b = 255, .a = 255 / 2});

	// draw score
	char *tmpString = malloc(9);
	snprintf(tmpString, 9, "%i", _score);
	drawText(tmpString, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.05, WHITE);

	drawCursor();

	// float heartScale = (getWidth() * 0.08) / _heartTex.width;
	// float healthBarScale = (getHeight() * 0.4) / _healthBarTex.height;
	// DrawTextureEx(_healthBarTex, (Vector2){.x = getWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale, .y = getHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale, WHITE);
	// DrawTextureEx(_heartTex, (Vector2){.x = getWidth() * 0.85, .y = getHeight() * (0.85 - _health / 250)}, 0, heartScale, WHITE);
	
	DrawRectangle(0, 0, getWidth(), getHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	snprintf(tmpString, 9, "%i", (int)(countDown - GetTime() + 1));
	float textSize = measureText(tmpString, getWidth() * 0.3);
	drawText(tmpString, getWidth() * 0.5 - textSize / 2, getHeight() * 0.3, getWidth() * 0.3, WHITE);
	free(tmpString);
	drawVignette();
}




#define HITPOINTAMOUNT 10

#define RippleAmount 10
#define addRipple(newRipple)                             \
	rippleEffect[rippleEffectIndex] = 0;                 \
	rippleEffectStrength[rippleEffectIndex] = newRipple; \
	rippleEffectIndex = (rippleEffectIndex + 1) % RippleAmount;
void fPlaying(bool reset)
{
	static float rippleEffect[RippleAmount] = {0};
	static float rippleEffectStrength[RippleAmount] = {0};
	static int rippleEffectIndex = 0;
	static float smoothHealth = 50;

	static float hitPointTimings [HITPOINTAMOUNT] = {0}; //how off they are
	static float hitPointsTimes [HITPOINTAMOUNT] = {0}; //when they're hit
	static int hitPointScores [HITPOINTAMOUNT] = {0}; //score for hit
	static int hitPointsIndex = 0;

	if(reset)
	{
		setMusicStart();
		_musicHead = 0;
		_transition = 0.1;
		_disableLoadingScreen = false;
		_musicPlaying = false;
		loadMap();
		for(int i = 0; i < _amountNotes; i++)
			_papNotes[i]->hit = false;

		//reset variables
		smoothHealth = 50;
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

	if(_settings.useMapZoom)
	{
		_wantedScrollSpeed = 4.2 / getTimingSignature(_musicHead).zoom;
	}

	_framesOffset = _settings.offset*48; //_offset in ms


	_musicHead += GetFrameTime() * _musicSpeed;
	_musicPlaying = true;
	fixMusicTime();

    float margin = _maxMargin * getMarginMod();


	_musicSpeed = getSpeedMod();

	if (endOfMusic())
	{
		stopMusic();
		readScore(_map);
		if (_map->highscore < _score || rankCalculation(_score, _combo, _notesMissed, _averageAccuracy) > _map->rank)
		{
			_mapRefresh = true; //to show new rank
			saveScore();
		}

		_pGameplayFunction = &fEndScreen;
		playAudioEffect(_finishSe);
		_transition = 0.1;
		return;
	}
	if (IsKeyDown(KEY_ESCAPE))
	{
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fPlaying;
	}

	ClearBackground(BLACK);

	drawBackground();

	DrawRectangle(0, 0, getWidth(), getHeight(), _fade);

	// draw ripples
	for (int i = 0; i < RippleAmount; i++)
	{
		if (rippleEffectStrength[i] == 0)
			continue;
		rippleEffect[i] += GetFrameTime() * 1200 * rippleEffectStrength[i];
		rippleEffectStrength[i] = fmax(rippleEffectStrength[i] - GetFrameTime() * 5, 0);
		float size = rippleEffect[i];
		DrawRing((Vector2){.x = musicTimeToScreen(_musicHead), .y = getHeight() * 0.5}, size * getWidth() * 0.001, size * 0.7 * getWidth() * 0.001, 0, 360, 50, ColorAlpha(WHITE, rippleEffectStrength[i] * 0.35));
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

		DrawCircle( musicTimeToScreen(_musicHead+hitPointTimings[i]), getHeight()*0.4, getWidth()*0.017, ColorAlpha(hpColor, alpha));
	}

	dNotes();

	if (_combo > _highestCombo)
		_highestCombo = _combo;

	static double lastHit = 0;
	float hitEffect = fmax(1-(GetTime()-lastHit)*5, 0);
	hitEffect *= hitEffect;

	if(_combo > 0)
	{
		char str[100];
		snprintf(str, 100, "%i", _combo);

		int x = musicTimeToScreen(_musicHead) - getWidth()*0.05;
		int y = getHeight() * 0.35;

		float size = getWidth()*(1+hitEffect*0.5+fmin(_combo, 200)/50.0)*0.02;

		x -= measureText(str, size);
		y -= size;

		drawText(str, x, y, size, WHITE);
	}
	// _isKeyPressed = false;
	// //do gameplay loop multiple times for higher polling (3 times per frame)
	// for(int i = 0; i < 3; i++)
	// {
	// 	WaitTime(1.0/MAXFPS/3);
	// 	PollInputEvents();
		if (_noteIndex < _amountNotes-1 && getMusicHead() - margin > _papNotes[_noteIndex]->time)
		{
			// passed note
			_noteIndex++;
			_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
			_combo = 0;
			playAudioEffect(_missSe);
		}
		// if (isAnyKeyDown())
		// 	_isKeyPressed = true;
		
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
					_combo = 0;
					_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
					_notesMissed++;
				}
				_averageAccuracy = ((_averageAccuracy * (_noteIndex - _notesMissed)) + ((1 / margin) * closestTime)) / (_noteIndex - _notesMissed + 1);
				int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / margin)) * _papNotes[_noteIndex]->health;
				_health += healthAdded * (1 / (getHealthMod() + 0.1));
				int scoreAdded = noLessThanZero(300 - closestTime * (300 / margin));
				if (scoreAdded > 200)
				{
					addRipple(1.5);
				}
				else if (scoreAdded > 100)
				{
					addRipple(1);
				}
				else
				{
					addRipple(0.6);
				}
				_score += scoreAdded * (1 + _combo / 100) * getScoreMod();
				_papNotes[_noteIndex]->hit = 1;
				_noteIndex++;
				_combo++;
				lastHit = GetTime();

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

				hitPointTimings[hitPointsIndex] = _papNotes[closestIndex]->time - _musicHead;
				hitPointsTimes[hitPointsIndex] = _musicHead;
				hitPointScores[hitPointsIndex] = scoreAdded;
				hitPointsIndex = (hitPointsIndex+1)%HITPOINTAMOUNT;

				if(_papNotes[closestIndex]->custSound && _settings.customAssets)
					playAudioEffect(_papNotes[closestIndex]->custSound->sound);
				else	
					playAudioEffect(_hitSe);
			}
			else
			{
				printf("missed note\n");
				_combo = 0;
				_health -= _missPenalty * getHealthMod() * _papNotes[_noteIndex]->health;
				playAudioEffect(_missHitSe);
				_notesMissed++;
			}
		}
	// }

	if (_health > 100)
		_health = 100;
	if (_health < 0)
		_health = 0;

	if (_health < 25)
		_fade = ColorAlpha(RED, 1 - _health / 25);
	else
		_fade = ColorAlpha(RED, 0);

	smoothHealth += (_health - smoothHealth) * GetFrameTime() * 3;

	float heartScale = (getWidth() * 0.08) / _heartTex.width;
	float healthBarScale = (getHeight() * 0.4) / _healthBarTex.height;
	DrawTextureEx(_healthBarTex, (Vector2){.x = getWidth() * 0.05 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale, .y = getHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale, WHITE);
	DrawTextureEx(_heartTex, (Vector2){.x = getWidth() * 0.05, .y = getHeight() * (0.85 - smoothHealth / 250)}, 0, heartScale, WHITE);

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
	snprintf(tmpString, 20, "%i", _score);
	drawText(tmpString, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.05, WHITE);

	// // draw combo
	// snprintf(tmpString, 20, "combo: %i", _combo);
	// drawText(tmpString, getWidth() * 0.70, getHeight() * 0.05, getWidth() * 0.05, WHITE);

	// draw acc
	snprintf(tmpString, 20, "%.1f%%", 100 * (1 - _averageAccuracy));
	drawText(tmpString, getWidth() * 0.85, getHeight() * 0.05, getWidth() * 0.04, WHITE);
	drawRank(getWidth()*0.75, getHeight()*0.03, getWidth()*0.1, getWidth()*0.1, rankCalculation(_score, _highestCombo, _notesMissed, _averageAccuracy));
	free(tmpString);
	// drawProgressBar();
}






void fEndScreen(bool reset)
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();
	DrawRectangle(0, 0, getWidth(), getHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});
	drawVignette();

	drawCSS("theme/endScreen.css");

	setCSS_VariableInt("highscore", _map->highscore);
	setCSS_VariableInt("highcombo", _map->combo);
	setCSS_VariableInt("highaccuracy", 100*(1-_map->accuracy));
	setCSS_VariableInt("highmisses", _map->misses);

	setCSS_VariableInt("score", _score);
	setCSS_VariableInt("combo", _highestCombo);
	setCSS_VariableInt("accuracy", 100 * (1 - _averageAccuracy));
	setCSS_VariableInt("misses", _notesMissed);

	CSS_Object * oldHighscoreObj = getCSS_ObjectPointer("oldHighscore");

	if(oldHighscoreObj)
		oldHighscoreObj->active = _map->highscore!=0;

	
	CSS_Object * newHighscoreObj = getCSS_ObjectPointer("newHighscore");

	if(newHighscoreObj)
		newHighscoreObj->active = (_map->highscore < _score);

	drawText("Rank", getWidth() * 0.55, getHeight() * 0.75, getWidth() * 0.05, WHITE);
	drawRank(getWidth()*0.7, getHeight()*0.65, getWidth()*0.2, getWidth()*0.2, rankCalculation(_score, _highestCombo, _notesMissed, _averageAccuracy));

	// if (interactableButton("Retry", 0.05, getWidth() * 0.15, getHeight() * 0.72, getWidth() * 0.3, getHeight() * 0.1))
	if(UIBUttonPressed("retryButton"))
	{
		// retrying map
		printf("retrying map! \n");

		_pGameplayFunction = &fCountDown;
		fCountDown(true);
		_musicHead = 0;
		_transition = 0.1;
	}
	// if (interactableButton("Main Menu", 0.05, getWidth() * 0.15, getHeight() * 0.85, getWidth() * 0.3, getHeight() * 0.1))

	if(UIBUttonPressed("mapSelectButton"))
	{
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fMapSelect;
		_mapRefresh = true;
		_transition = 0.1;
	}
	drawCursor();
}




void fFail(bool reset)
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();
	drawVignette();

	drawCSS("theme/fail.css");

	setCSS_VariableInt("score", _score);
	setCSS_VariableInt("missed", _notesMissed);
	setCSS_VariableInt("combo", _combo);
	setCSS_VariableInt("accuracy", 100*(1-_averageAccuracy));

	if (UIBUttonPressed("retryButton"))
	{
		// retrying map
		printf("retrying map! \n");
		_pGameplayFunction = &fCountDown;
		fCountDown(true);
		_musicHead = 0;
		_transition = 0.7;
	}
	if (UIBUttonPressed("exitButton"))
	{
		// exiting map
		printf("going to main Menu! \n");
		unloadMap();
		gotoMainMenu(false);
		_pNextGameplayFunction = &fPlaying;
		_transition = 0.1;
	}
	drawCursor();
}