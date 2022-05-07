#ifndef MAINC
#include "drawing.h"
#endif
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/raylib.h"
Texture2D _cursorTex;
Texture2D _noteTex;
Texture2D _healthBarTex;
Texture2D _heartTex;
Texture2D _background;

Color _UIColor = WHITE;

//TODO probably move this to drawing?
Color _fade = WHITE;

extern float _musicHead, _scrollSpeed, *_pNotes;
extern int _noteIndex, _amountNotes, _clickPressSE_Size, _clickReleaseSE_Size;
extern bool _noBackground;
extern void * _pMusic, *_pClickPress, *_pClickRelease;
extern float _transition;


void drawTransition()
{
	if(_transition < 1)
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, GetFrameTime()*5));
	else
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, fmax(2-_transition, 0)));
}

float musicTimeToScreen(float musicTime)
{
	float middle = GetScreenWidth() / 2;
	return middle + middle * (musicTime - _musicHead) * (1 / _scrollSpeed);
}

float screenToMusicTime(float x)
{
	float middle = GetScreenWidth() / 2;
	return (x - middle) / (middle * (1 / _scrollSpeed)) + _musicHead;
}

float noteFadeOut(float note)
{
	return fmin(1, fmax(0, (1-fabs(note - _musicHead) * (1/_scrollSpeed))*3));
}

void drawCursor ()
{
	static float lastClick;
	if(!IsCursorOnScreen())
		return;
	if(IsMouseButtonPressed(0))
	{
		printf("pressed \n");
		playAudioEffect(_pClickPress, _clickPressSE_Size);
	}else if(IsMouseButtonReleased(0))
	{
		printf("released \n");
		playAudioEffect(_pClickRelease, _clickReleaseSE_Size);
	}
	if(IsMouseButtonDown(0))
	{
		lastClick = GetTime();
	}
	float size = (GetScreenWidth() * (0.06 - 0.1 * noLessThanZero(0.06 + (lastClick - GetTime())) )) / _cursorTex.width;
	float xOffset = _cursorTex.width*size*0.3;
	float yOffset = _cursorTex.height*size*0.2;
	DrawTextureEx(_cursorTex, (Vector2){.x=GetMouseX()-xOffset, .y=GetMouseY()-yOffset}, 0, size, WHITE);
}

void dNotes () 
{
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;
	

	DrawRectangleGradientH(0,0 , middle - width / 2, GetScreenHeight(), ColorAlpha(BLACK, 0.6), ColorAlpha(BLACK, 0.3));
	for(int i = _noteIndex; i >= 0 && _pNotes[i] + _scrollSpeed > _musicHead; i--)
	{
		if(i < 0) continue;
		//DrawCircle( middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
		//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
		DrawTextureEx(_noteTex, (Vector2){.x=musicTimeToScreen(_pNotes[i])- _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=128,.g=128,.b=128,.a=255*noteFadeOut(_pNotes[i])});

	}

	if(_noteIndex < _amountNotes) //draw notes after line
	{
		//draw notes before line
		for(int i = _noteIndex; i < _amountNotes && _pNotes[i] - _scrollSpeed < _musicHead; i++)
		{
			//DrawCircle( middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(_noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			DrawTextureEx(_noteTex, (Vector2){.x=musicTimeToScreen(_pNotes[i])- _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=255,.g=255,.b=255,.a=255*noteFadeOut(_pNotes[i])});

		}
	}
	DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=255/2});
}

void drawMapThumbnail(Rectangle rect, Map *map)
{
	float imageRatio = 0.7;
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonReleased(0))
		color = GRAY;

	DrawRectangle(rect.x, rect.y, rect.width, rect.height, color);

	DrawRectangle(rect.x, rect.y, rect.width, rect.height*imageRatio, BLACK);

	//todo, proper scaling of thumbnails
	// float imageScaling = rect.width/map.image.width;
	// float imageOffset = rect.width - map.image.width*imageScaling/2;
	// DrawTextureRec(map.image, (Rectangle){.x=0, .y=0, .width=map.image.width, .height=map.image.height}, (Vector2){.x=rect.x, .y=rect.y}, color);
	DrawTexturePro(map->image, (Rectangle){.x=0, .y=0, .width=map->image.width, .height=map->image.height}, (Rectangle){.x=rect.x, .y=rect.y, .width=rect.width, .height=rect.height*imageRatio},
			(Vector2){.x=0,.y=0}, 0, color);
	
	char text [100];
	sprintf(text, "%s - %s", map->name, map->creator);
	int textSize = MeasureText(text, GetScreenWidth() * 0.05);
	DrawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + GetScreenHeight() * 0.01+rect.height*imageRatio, GetScreenWidth() * 0.04, BLACK);
}

void drawBackground()
{
	if(!_noBackground)
	{
		float scale = (float)GetScreenWidth() / (float)_background.width;
		DrawTextureEx(_background, (Vector2){.x=0, .y=(GetScreenHeight() - _background.height * scale)/2}, 0, scale,WHITE);
	}else{
		DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
	}
}
void resetBackGround()
{
	if(_background.id != 0)
		UnloadTexture(_background);
	_background = LoadTexture("background.png");
}
void drawBars()
{
	//Draw the bars
	float middle = GetScreenWidth()/2;
	float distBetweenBeats = getMusicDuration() / getBeatsCount();

	float distBetweenBars = distBetweenBeats*4;
	for (int i = screenToMusicTime(0)/distBetweenBars; i < screenToMusicTime(GetScreenWidth())/distBetweenBars; i++)
	{
		DrawRectangle(musicTimeToScreen(distBetweenBars*i),GetScreenHeight()*0.6,GetScreenWidth()*0.01,GetScreenHeight()*0.3,(Color){.r=255,.g=255,.b=255,.a=180});
	}

	for (int i = screenToMusicTime(0)/distBetweenBeats; i < screenToMusicTime(GetScreenWidth())/distBetweenBeats; i++)
	{
		if(i % 4 == 0) continue;
		DrawRectangle(musicTimeToScreen(distBetweenBeats*i),GetScreenHeight()*0.7,GetScreenWidth()*0.01,GetScreenHeight()*0.2,(Color){.r=255,.g=255,.b=255,.a=180});
	}
}

void drawMusicGraph(float transparent)
{
	if(_pMusic == 0)
		return;

	//music stuff
	float beginning = screenToMusicTime(0);
	float end = screenToMusicTime(getMusicDuration());
	int amountBars = GetScreenWidth()/2;
	float timePerBar = (end-beginning)/amountBars;
	int samplesPerBar = getSamplePosition(timePerBar);
	int sampleBegin = getSamplePosition(beginning);

	//drawing stuff
	int pixelsPerBar = GetScreenWidth()/amountBars;
	float scaleBar = GetScreenHeight()*0.2;
	//looping through all the bars / samples
	for(int i = 0; i < amountBars; i++)
	{
		float highest = 0;
		int sampleHead = sampleBegin + samplesPerBar*i;
		for(int j = 0; j < samplesPerBar; j++)
			if(sampleHead+j > 0 && fabs(((float*)_pMusic)[sampleHead+j]) > highest)
				highest = fabs(((float*)_pMusic)[sampleHead+j]);

		DrawRectangle(i*pixelsPerBar, GetScreenHeight()-highest*scaleBar, pixelsPerBar, highest*scaleBar, ColorAlpha(WHITE, transparent));
	}

}

void drawVignette()
{
	DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight()*0.3, ColorAlpha(BLACK, 0.7), ColorAlpha(BLACK, 0));
	DrawRectangleGradientV(0, GetScreenHeight()*0.7, GetScreenWidth(), GetScreenHeight()*0.3, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.7));
}

//TODO don't do this, pls fix
void drawProgressBar() {drawProgressBarI(false);}

void drawProgressBarI(bool interActable)
{
	static bool isGrabbed = false;
	DrawRectangle( GetScreenWidth()*0.01, GetScreenHeight()*0.93, GetScreenWidth()*0.98, GetScreenHeight()*0.02, (Color){.r=_UIColor.r,.g=_UIColor.g,.b=_UIColor.b,.a=126});
	//drop shadow
	DrawCircle(getMusicPosition()/ getMusicDuration()*GetScreenWidth(), GetScreenHeight()*0.945, GetScreenWidth()*0.025, (Color){.r=0,.g=0,.b=0,.a=80});
	DrawCircle(getMusicPosition()/ getMusicDuration()*GetScreenWidth(), GetScreenHeight()*0.94, GetScreenWidth()*0.025, _UIColor);
	char str[50];
	sprintf(str, "%i:%i/%i:%i", (int)floor(getMusicPosition()/60), (int)getMusicPosition()%60, (int)floor(getMusicDuration()/60), (int)getMusicDuration()%60);
	DrawText(str, GetScreenWidth()*0.85, GetScreenHeight()*0.85, GetScreenWidth()*0.03,_UIColor);
	if(interActable)
	{
		float x = getMusicPosition()/ getMusicDuration()*GetScreenWidth();
		float y = GetScreenHeight()*0.94;
		if(fDistance(x, y, GetMouseX(), GetMouseY()) < GetScreenWidth()*0.03 && IsMouseButtonDown(0) || isGrabbed)
		{
			//printf("grabbed %.2f  ", _musicHead);
			isGrabbed = true;
			_musicHead = clamp(GetMouseX()/(float)GetScreenWidth()*getMusicDuration(), 0, getMusicDuration());
			//printf("%.2f \n", _musicHead);
		}
		if(!IsMouseButtonDown(0))
			isGrabbed = false;
	}
}

void drawButton(Rectangle rect, char * text, float fontScale)
{
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonDown(0))
		color = GRAY;
	DrawRectangle(rect.x, rect.y, rect.width, rect.height, color);
	int textSize = MeasureText(text, GetScreenWidth() * fontScale);
	DrawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + GetScreenHeight() * 0.01, GetScreenWidth() * fontScale, (color.r == GRAY.r) ? BLACK : DARKGRAY);
}
