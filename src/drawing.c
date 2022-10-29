
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../deps/raylib/src/raylib.h"


#define EXTERN_MAIN
#define EXTERN_GAMEPLAY
#define EXTERN_AUDIO
#define EXTERN_EDITOR

#include "drawing.h"

#include "audio.h"
#include "files.h"
#include "main.h"

#include "gameplay/editor.h"
#include "gameplay/gameplay.h"
#include "gameplay/menus.h"


Texture2D _cursorTex;
Texture2D _noteTex;
Texture2D _healthBarTex;
Texture2D _heartTex;
Texture2D _background, _menuBackground;
Texture2D _buttonTile[3][3];

Font _font;

Color _UIColor = WHITE;

Color _fade = WHITE;


int measureText (char * text, int fontSize)
{
	return MeasureTextEx(_font, text, fontSize, fontSize*0.1).x;
}

void drawText(char * str, int x, int y, int fontSize, Color color)
{
	DrawTextEx(_font, str, (Vector2){.x=x, .y=y}, fontSize, fontSize*0.1, color);
}

void drawTransition()
{
	if(_transition < 1)
		DrawRectangle(0, 0, getWidth(), getHeight(), ColorAlpha(BLACK, GetFrameTime()*5));
	else
		DrawRectangle(0, 0, getWidth(), getHeight(), ColorAlpha(BLACK, fmax(2-_transition, 0)));
}

float musicTimeToScreen(float musicTime)
{
	float middle = getWidth() / 2;
	float position = getWidth() * 0.4;
	return position + middle * (musicTime - getMusicHead()) * (1 / _scrollSpeed);
}

float screenToMusicTime(float x)
{
	float middle = getWidth() / 2;
	float position = getWidth() * 0.4;
	return (x - position) / (middle * (1 / _scrollSpeed)) + getMusicHead();
}

float noteFadeOut(float note)
{
	if(_musicHead < note)
		return fmin(1, fmax(0, 1 - (musicTimeToScreen(note) - musicTimeToScreen(_musicHead)) / (getWidth()*1)));
	return fmin(1, fmax(0, 1 - (musicTimeToScreen(_musicHead) - musicTimeToScreen(note) ) / (getWidth()*0.4)));
	
}

void drawCursor ()
{
	static float lastClick;
	if(!IsCursorOnScreen())
		return;
	if(IsMouseButtonPressed(0))
	{
		printf("pressed \n");
		playAudioEffect(_clickPressSe);
	}else if(IsMouseButtonReleased(0))
	{
		printf("released \n");
		playAudioEffect(_clickReleaseSe);
	}
	if(IsMouseButtonDown(0))
	{
		lastClick = GetTime();
	}
	float size = (getWidth() * (0.06 - 0.1 * noLessThanZero(0.06 + (lastClick - GetTime())) )) / _cursorTex.width;
	float xOffset = _cursorTex.width*size*0.3;
	float yOffset = _cursorTex.height*size*0.2;
	DrawTextureEx(_cursorTex, (Vector2){.x=GetMouseX()-xOffset, .y=GetMouseY()-yOffset}, 0, size, WHITE);
}

void drawNote(float musicTime, Note * note, Color color, float customScaling)
{
	float scaleNotes = (float)(getWidth() / _noteTex.width) / 9;

	if(note->hit)
		return;
	Texture tex = _noteTex;
	if(note->custTex)
	{
		tex = note->custTex->texture;
	}

	if(customScaling != 0)
		scaleNotes = customScaling;

	scaleNotes *= (_settings.noteSize / 10.0);
	
	if(!note->anim)
		DrawTextureEx(tex, (Vector2){.x=musicTimeToScreen(note->time)- tex.width * scaleNotes / 2, .y=getHeight() / 2 -tex.height * scaleNotes/2}, 0,  scaleNotes, color);
	else if(note->animSize)
	{
		if(_pGameplayFunction == &fEditor)
			DrawTextureEx(tex, (Vector2){.x=musicTimeToScreen(note->time)- tex.width * scaleNotes / 2, .y=getHeight() / 2 -tex.height * scaleNotes/2}, 0,  scaleNotes, ColorAlpha(GRAY, 0.5));
		//draw animated note
		float time = (musicTime - note->time) / (_scrollSpeed);
		// if(time < note->anim[0].time || time < time < note->anim[note->animSize].time)
		// 	return;
		int anim = 0;
		for(int i = 0; i < note->animSize-1; i++)
		{
			if(note->anim[i].time*2-1 < time && note->anim[i+1].time * 2 - 1 > time)
			{
				anim = i;
				break;
			}
		}
		Frame frame1 = note->anim[anim];
		Frame frame2 = note->anim[anim+1];
		float time1 = frame1.time*2-1;
		float time2 = frame2.time*2-1;
		float betweenFrames = (time - time1) / (time2 - time1);
		Vector2 pos;
		pos.x = frame1.vec.x + (frame2.vec.x-frame1.vec.x)*betweenFrames;
		pos.y = frame1.vec.y + (frame2.vec.y-frame1.vec.y)*betweenFrames;
		DrawTextureEx(tex, (Vector2){.x=pos.x*getWidth() - tex.width * scaleNotes / 2, .y=pos.y*getHeight() - tex.height * scaleNotes/2}, 0,  scaleNotes, color);
	}
}

void dNotes () 
{
	static float fade = 0;
	float width = getWidth() * 0.01;
	float position = musicTimeToScreen(_musicHead);
	
	DrawRectangle(0, getHeight()*0.35, getWidth(), getHeight()*0.3, ColorAlpha(BLACK, 0.4));
	DrawRectangleGradientH(0,0 , position - width / 2, getHeight(), ColorAlpha(BLACK, 0.6), ColorAlpha(BLACK, 0.3));
	for(int i = _noteIndex >= _amountNotes ? _amountNotes-1 : _noteIndex; i >= 0 && i < _amountNotes && musicTimeToScreen(_papNotes[i]->time) > 0; i--)
	{
		if(i < 0) continue;

		float closestNote = 999;
		if(i != 0)
			closestNote = _papNotes[i]->time - _papNotes[i-1]->time;
		if(i != _amountNotes-1)
			closestNote = fmin(closestNote, _papNotes[i+1]->time - _papNotes[i]->time);

		float customScale = 0;

		float maxDistance = _scrollSpeed * _maxMargin*2;

		if(closestNote < maxDistance)
		{
			customScale = (closestNote/maxDistance) * (getWidth() / _noteTex.width) / 9;
		}

		drawNote(_musicHead, _papNotes[i], ColorAlpha(GRAY, noteFadeOut(_papNotes[i]->time)), customScale);

	}

	//draw notes before line
	for(int i = _noteIndex >= _amountNotes ? _amountNotes-1 : _noteIndex; i < _amountNotes && i>= 0 && musicTimeToScreen(_papNotes[i]->time) < getWidth(); i++)
	{
		float closestNote = 999;
		if(i != 0)
			closestNote = _papNotes[i]->time - _papNotes[i-1]->time;
		if(i != _amountNotes-1)
			closestNote = fmin(closestNote, _papNotes[i+1]->time - _papNotes[i]->time);

		float customScale = 0;

		float maxDistance = _scrollSpeed * _maxMargin*2;

		if(closestNote < maxDistance)
		{
			customScale = (closestNote/maxDistance) * (getWidth() / _noteTex.width) / 9;
		}

		drawNote(_musicHead, _papNotes[i], ColorAlpha(WHITE, noteFadeOut(_papNotes[i]->time)), customScale);

	}

	DrawRectangle(position - width / 2,0 , width, getHeight(), ColorAlpha(WHITE, 0.5*fade));
	if(_isKeyPressed)
		fade = 1;
	fade -= GetFrameTime()*10;
}

void drawRank(int x, int y, int width, int height, float accuracy)
{
	//rank
	Color colRank = LIGHTGRAY;
	char * text = "F";
	if(accuracy < 0.7)
	{
		colRank = WHITE;
		text = "D";
	}
	if(accuracy < 0.4)
	{
		colRank = GREEN;
		text = "C";
	}
	if(accuracy < 0.3)
	{
		colRank = YELLOW;
		text = "B";
	}
	if(accuracy < 0.2)
	{
		colRank = ORANGE;
		text = "A";
	}
	if(accuracy < 0.1)
	{
		colRank = RED;
		text = "S";
	}
	DrawTexturePro(_noteTex, (Rectangle){.x=0, .y=0, .width=_noteTex.width, .height=_noteTex.height}, (Rectangle){.x=x, .y=y, .width=width, .height=height}, (Vector2) {.x=0, .y=0}, 0, colRank);
	drawText(text, x+width*0.2, y+height*0.03, width, WHITE);
}

void drawMapThumbnail(Rectangle rect, Map *map, int highScore, int combo, float accuracy, bool selected)
{
	if(map->image.id == 0)
	{
		//load map image onto gpu(cant be loaded in sperate thread because opengl >:( )
		if(map->cpuImage.width > 0)
		{
			map->image = LoadTextureFromImage(map->cpuImage);
			UnloadImage(map->cpuImage);
			map->cpuImage.width = -1;
			if(map->image.id == 0)
				map->image.id = -1; 
			else
				SetTextureFilter(map->image, TEXTURE_FILTER_BILINEAR);
		}
	}
	float imageRatio = 0.8;
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonReleased(0))
		color = GRAY;

	DrawRectangle(rect.x, rect.y, rect.width, rect.height, color);

	DrawRectangle(rect.x, rect.y, rect.width, rect.height*imageRatio, BLACK);

	float ogImageRatio = map->image.width / (float)map->image.height;
	float newImageRatio = rect.width / (rect.height*imageRatio);
	Vector2 imageScaling = {.x=rect.width,.y=rect.height*imageRatio}, imageOffset = {.x=0,.y=0};
	if(ogImageRatio>newImageRatio)
	{
		imageScaling.y = ((float)map->image.height/ map->image.width)*imageScaling.x;
		imageOffset.y = (rect.height*imageRatio)/2-imageScaling.y/2;
	}else {
		imageScaling.x = ogImageRatio*imageScaling.y;
		imageOffset.x = rect.width/2-imageScaling.x/2;
	}

	DrawTexturePro(map->image, (Rectangle){.x=0, .y=0, .width=map->image.width, .height=map->image.height}, (Rectangle){.x=rect.x+imageOffset.x, .y=rect.y+imageOffset.y, .width=imageScaling.x, .height=imageScaling.y},
			(Vector2){.x=0,.y=0}, 0, color);

	DrawRectangleGradientV(rect.x, rect.y+rect.height*0.4, rect.width, rect.height*imageRatio-rect.height*0.4, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.5));
	
	char text [100];
	snprintf(text, 100, "%s - %s", map->name, map->artist);
	int length = strlen(text);
	char* textPointer = text;
	if(length > 18)
	{
		if(mouseInRect(rect)) { //scroll the text when hovering
			int offset = (int)floor(GetTime()*1.5)%(length-17);
			textPointer += offset;
			text[offset+17] = '\0';
		} else {
			text[16] = '.';
			text[17] = '.';
			text[18] = '.';
			text[19] = '\0';
		}
	}
	int textSize = measureText(textPointer, getWidth() * 0.03);
	drawText(textPointer, rect.x + rect.width/2 - textSize / 2, rect.y + getHeight() * 0.01+rect.height*imageRatio, getWidth() * 0.03, DARKGRAY);
	
	snprintf(text, 100, "%i", map->difficulty);
	DrawRectangle(rect.x + rect.width*0.13, rect.y + 0.60*rect.height, rect.width*0.2, rect.height*0.20, ColorAlpha(BLACK, 0.4));
	drawText(text, rect.x + rect.width*0.08, rect.y + 0.60*rect.height, getWidth() * 0.02625, WHITE);
	snprintf(text, 100, "%i:%i", (int)floorf(map->musicLength/60), (int)floorf(map->musicLength-floorf(map->musicLength/60)*60));
	drawText(text, rect.x + rect.width*0.06, rect.y + 0.68*rect.height, getWidth() * 0.02625, WHITE);

	snprintf(text, 100, "%i", highScore);
	if(highScore !=0)
	{
		DrawRectangle(rect.x + rect.width*0.78, rect.y + 0.02*rect.height, rect.width*0.2, rect.height*0.20, ColorAlpha(BLACK, 0.4));
		drawText(text, rect.x + rect.width*0.80, rect.y + 0.02*rect.height, getWidth() * 0.015, WHITE);
		snprintf(text, 100, "%i", combo);
		drawText(text, rect.x + rect.width*0.82, rect.y + 0.08*rect.height, getWidth() * 0.015, WHITE);
		snprintf(text, 100, "%.2f", 100*(1-accuracy));
		drawText(text, rect.x + rect.width*0.84, rect.y + 0.14*rect.height, getWidth() * 0.015, WHITE);
		drawRank(rect.x+rect.width*0.03, rect.y+rect.width*0.03, rect.width*0.08, rect.width*0.08, accuracy);

	}
}

void drawBackground()
{
	if(!_noBackground)
	{
		float scale = (float)getWidth() / (float)_background.width;
		DrawTextureEx(_background, (Vector2){.x=0, .y=(getHeight() - _background.height * scale)/2}, 0, scale,WHITE);
	}else{
		DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = getHeight(), .width= getWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
	}
}

void resetBackGround()
{
	_background = _menuBackground;
}

void drawActualBars(TimingSegment timseg)
{
	//Draw the bars
	double distBetweenBeats = (60.0/timseg.bpm) / _barMeasureCount;
	if(timseg.beats < 1)
		return;
	double distBetweenBars = distBetweenBeats*timseg.beats;
	for (int i = (screenToMusicTime(0)-timseg.time)/distBetweenBars; i < (screenToMusicTime(getWidth())-timseg.time)/distBetweenBars; i++)
	{
		DrawRectangle(musicTimeToScreen(distBetweenBars*i+timseg.time)-10,getHeight()*0.6,getWidth()*0.01,getHeight()*0.3,(Color){.r=255,.g=255,.b=255,.a=180});
	}

	for (int i = (screenToMusicTime(0)-timseg.time)/distBetweenBeats; i < (screenToMusicTime(getWidth())-timseg.time)/distBetweenBeats; i++)
	{
		if(i % timseg.beats == 0) continue;
		DrawRectangle(musicTimeToScreen(distBetweenBeats*i+timseg.time)-10,getHeight()*0.7,getWidth()*0.01,getHeight()*0.2,(Color){.r=255,.g=255,.b=255,.a=180});
	}
}


void drawBars()
{
	

	if(_amountTimingSegments == 0 || !_paTimingSegment)
	{
		drawActualBars((TimingSegment){.bpm=_map->bpm, .time=_map->offset/1000.0, .beats=_map->beats});
		return;
	}

	int firstSeg = _amountTimingSegments-1;
	int lastSeg = 0;

	float beginTime = screenToMusicTime(0);
	float endTime = screenToMusicTime(getWidth());

	for(int i = 0; i < _amountTimingSegments; i++)
	{
		float time = _paTimingSegment[i].time;
		if(time < beginTime || time > endTime)
			continue;
		
		if(time < _paTimingSegment[firstSeg].time)
			firstSeg = i;
		if(time > _paTimingSegment[lastSeg].time)
			lastSeg = i;
	}

	firstSeg--; //we must get the one out of screen which still has an effect
	if(firstSeg < 0)
		firstSeg = 0;

	BeginScissorMode(0, 0, fmax(musicTimeToScreen(_paTimingSegment[firstSeg].time), 0), getHeight());
		drawActualBars((TimingSegment){.bpm=_map->bpm, .time=_map->offset/1000.0, .beats=_map->beats});
	EndScissorMode(); 
	

	for(int part = firstSeg; part < lastSeg; part++)
	{
		BeginScissorMode(fmax(musicTimeToScreen(_paTimingSegment[part].time),0 ), 0,
				fmax( 0, fmax(musicTimeToScreen(_paTimingSegment[part+1].time),0)-fmax( 0, musicTimeToScreen(_paTimingSegment[part].time))),
				getHeight());
			drawActualBars(_paTimingSegment[part]);
		EndScissorMode();
	}

	if(musicTimeToScreen(_paTimingSegment[lastSeg].time) < 0)
	{
		for(int i = 0; i < _amountTimingSegments; i++)
			if(_paTimingSegment[i].time > _paTimingSegment[lastSeg].time && _paTimingSegment[i].time < screenToMusicTime(0))
				lastSeg = i;
	}

	BeginScissorMode(fmax( 0, musicTimeToScreen(_paTimingSegment[lastSeg].time)), 0, getWidth(), getHeight());
		drawActualBars(_paTimingSegment[lastSeg]);
	EndScissorMode();
}

void drawMusicGraph(float transparent)
{
	if(!_pMusic || !_pMusic->size || !_pMusic->data)
		return;
	
	//music stuff
	float beginning = screenToMusicTime(0);
	float end = screenToMusicTime(getWidth());
	int amountBars = getWidth()/2;
	float timePerBar = (end-beginning)/amountBars;
	int samplesPerBar = getSamplePosition(timePerBar);
	int sampleBegin = getSamplePosition(beginning);

	//drawing stuff
	float pixelsPerBar = getWidth()/(float)amountBars;
	float scaleBar = getHeight()*0.2;
	//looping through all the bars / samples
	for(int i = 0; i < amountBars; i++)
	{
		float highest = 0;
		int sampleHead = sampleBegin + samplesPerBar*i;
		for(int j = 0; j < samplesPerBar; j++)
			if(sampleHead+j > 0 && fabs(((float*)_pMusic->data)[sampleHead+j]) > highest)
				highest = fabs(((float*)_pMusic->data)[sampleHead+j]);

		DrawRectangle(i*pixelsPerBar, getHeight()-highest*scaleBar, pixelsPerBar, highest*scaleBar, ColorAlpha(WHITE, transparent));
	}

}

void drawVignette()
{
	DrawRectangleGradientV(0, 0, getWidth(), getHeight()*0.3, ColorAlpha(BLACK, 0.5), ColorAlpha(BLACK, 0));
	DrawRectangleGradientV(0, getHeight()*0.7, getWidth(), getHeight()*0.3, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.5));
}

//TODO don't do this, pls fix
// no
void drawProgressBar() {drawProgressBarI(false);}

bool drawProgressBarI(bool interActable)
{
	static bool isGrabbed = false;
	DrawRectangle( getWidth()*0.01, getHeight()*0.93, getWidth()*0.98, getHeight()*0.02, (Color){.r=_UIColor.r,.g=_UIColor.g,.b=_UIColor.b,.a=126});
	//drop shadow
	DrawCircle(getMusicPosition()/ getMusicDuration()*getWidth(), getHeight()*0.945, getWidth()*0.025, (Color){.r=0,.g=0,.b=0,.a=80});
	DrawCircle(getMusicPosition()/ getMusicDuration()*getWidth(), getHeight()*0.94, getWidth()*0.025, _UIColor);
	char str[50];
	snprintf(str, 50, "%i:%i/%i:%i", (int)floor(getMusicPosition()/60), (int)getMusicPosition()%60, (int)floor(getMusicDuration()/60), (int)getMusicDuration()%60);
	drawText(str, getWidth()*0.85, getHeight()*0.85, getWidth()*0.03,_UIColor);
	if(interActable)
	{
		float x = getMusicPosition()/ getMusicDuration()*getWidth();
		float y = getHeight()*0.94;
		if((fDistance(x, y, GetMouseX(), GetMouseY()) < getWidth()*0.03 && IsMouseButtonDown(0)) || isGrabbed)
		{
			isGrabbed = true;
			_musicHead = clamp(GetMouseX()/(float)getWidth()*getMusicDuration(), 0, getMusicDuration());
		}
		if(!IsMouseButtonDown(0))
			isGrabbed = false;
		return isGrabbed;
	}
	return false;
}


void drawBox(Rectangle rect, Color color)
{
	double size = getWidth()*0.02;
	while(rect.height < size*2)
		size *=0.5;

	int sectionsX = rect.width/size;
	int sectionsY = rect.height/size;
	double secSizeX = rect.width/sectionsX;
	double secSizeY = rect.height/sectionsY;

	//it's not done in a loop/ fully tiled because mid sections can be stretched making for better fits
	//corners
	Rectangle dest = (Rectangle){.x=rect.x, .y=rect.y, .width=secSizeX, .height=secSizeY+1};
	Rectangle source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[0][0], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x+secSizeX*(sectionsX-1), .y=rect.y, .width=secSizeX, .height=secSizeY+1};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[2][0], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x, .y=rect.y+secSizeY*(sectionsY-1), .width=secSizeX, .height=secSizeY+1};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[0][2], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x+secSizeX*(sectionsX-1), .y=rect.y+secSizeY*(sectionsY-1), .width=secSizeX, .height=secSizeY+1};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[2][2], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	//edges

	dest = (Rectangle){.x=rect.x+secSizeX, .y=rect.y, .width=secSizeX*(sectionsX-2), .height=secSizeY+1};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[1][0], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x, .y=rect.y+secSizeY, .width=secSizeX, .height=secSizeY*(sectionsY-2)};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[0][1], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	//bottom
	dest = (Rectangle){.x=rect.x+secSizeX, .y=rect.y+secSizeY*(sectionsY-1), .width=secSizeX*(sectionsX-2), .height=secSizeY+1};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[1][2], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x+secSizeX*(sectionsX-1), .y=rect.y+secSizeY, .width=secSizeX, .height=secSizeY*(sectionsY-2)};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[2][1], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	

	//fill
	dest = (Rectangle){.x=rect.x+secSizeX, .y=rect.y+secSizeY, .width=secSizeX*(sectionsX-2), .height=secSizeY*(sectionsY-2)};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[1][1], source, dest, (Vector2){.x=0, .y=0}, 0, color);



	//fully tiled method v

	// for(int y = 0; y < sectionsY; y++)
	// 	for(int x = 0; x < sectionsX; x++)
	// 	{
	// 		int sX = 1;
	// 		int sY = 1;
	// 		if(x == 0)
	// 			sX = 0;
	// 		if(x == sectionsX-1)
	// 			sX = 2;
	// 		if(y == 0)
	// 			sY = 0;
	// 		if(y == sectionsY-1)
	// 			sY = 2;
	// 		Rectangle source = (Rectangle){.x=0, .y=0, .width=_buttonTile[sX][sY].width, .height=_buttonTile[sX][sY].height};
	// 		Rectangle dest = (Rectangle){.x=rect.x+secSizeX*x, .y=rect.y+secSizeY*y, .width=secSizeX, .height=secSizeY};
	// 		DrawTexturePro(_buttonTile[sX][sY], source, dest, (Vector2){.x=0, .y=0}, 0, color);
	// 	}
}

void drawButton(Rectangle rect, char * text, float fontScale)
{
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonDown(0))
		color = GRAY;

	drawBox(rect, color);
	fontScale *= 1.3;
	// DrawRectangle(rect.x, rect.y, rect.width, rect.height, ColorAlpha(color, 0.5));
	int screenSize = getWidth() > getHeight() ? getHeight() : getWidth();
	int textSize = measureText(text, screenSize * fontScale);
	drawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + rect.height*0.15, screenSize * fontScale, (color.r == GRAY.r) ? BLACK : DARKGRAY);
}

void drawHint(Rectangle rect, char * text)
{
	static float timerShow = 0;
	if(!mouseInRect(rect))
	{
		timerShow = 0;
	}

	timerShow += GetFrameTime();

	if(timerShow < 0.3f)
		return;

	float fontScale = getWidth()*0.03;
	// DrawRectangle(rect.x, rect.y, rect.width, rect.height, ColorAlpha(color, 0.5));
	int textSize = measureText(text, fontScale);
	Vector2 pos = GetMousePosition();
	pos.x += getWidth()*0.04;
	DrawRectangle(pos.x-getWidth()*0.01, pos.y, textSize+getWidth()*0.02, getHeight()*0.05, ColorAlpha(BLACK, 0.6));
	drawText(text, pos.x, pos.y, fontScale, WHITE);
}

void drawLoadScreen()
{
	static float angle = 0;
	angle += GetFrameTime()*(sinf(GetTime()*4)+1.5)*300;

	DrawTextureTiled(_menuBackground, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _menuBackground.height, .width = _menuBackground.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, ColorAlpha(WHITE, _loadingFade));
	// DrawTextureTiled(_menuBackground, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		// (Rectangle){.x=0, .y=0, .height = getHeight(), .width= getWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, ColorAlpha(WHITE, _loadingFade));
	DrawRing((Vector2){.x=getWidth()/2, .y=getHeight()/2}, getWidth()*0.1, getWidth()*0.15, angle, angle+170, 50, ColorAlpha(WHITE, _loadingFade));
	if(_loadingFade== 1)
	{
		drawVignette();
		drawCursor();
	}
}

void drawButtonNoSprite(Rectangle rect, char * text, float fontScale)
{
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonDown(0))
		color = GRAY;

	DrawRectangle(rect.x, rect.y, rect.width, rect.height, color);
	fontScale *= 1.3;
	// DrawRectangle(rect.x, rect.y, rect.width, rect.height, ColorAlpha(color, 0.5));
	int screenSize = getWidth() > getHeight() ? getHeight() : getWidth();
	int textSize = measureText(text, screenSize * fontScale);
	drawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + rect.height*0.2, screenSize * fontScale, (color.r == GRAY.r) ? BLACK : DARKGRAY);
}

void initDrawing()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	if(_settings.fullscreen)
	{
		// SetConfigFlags(FLAG_FULLSCREEN_MODE);
		InitWindow(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()), "One Button Rythm");
		ToggleFullscreen();
	}else
		InitWindow(_settings.resolutionX, _settings.resolutionY, "One Button Rythm");
	
	SetWindowIcon(LoadImage("assets/note.png"));
	SetTargetFPS(360);
	SetExitKey(0);

	HideCursor();
	SetConfigFlags(FLAG_MSAA_4X_HINT);

	_loadingFade = 0.3;

	_menuBackground = LoadTexture("assets/background.png");
	SetTextureFilter(_menuBackground, TEXTURE_FILTER_BILINEAR);

	_loadingFade += 0.1;
	BeginDrawing();
		ClearBackground(BLACK);
		DrawText("Loading", getWidth()*0.4, getHeight()*0.4, getWidth()*0.05, WHITE);
		// drawLoadScreen();
	EndDrawing();

	_heartTex = LoadTexture("assets/heart.png");
	SetTextureFilter(_heartTex, TEXTURE_FILTER_ANISOTROPIC_8X);
	_healthBarTex = LoadTexture("assets/healthBar.png");
	SetTextureFilter(_healthBarTex, TEXTURE_FILTER_ANISOTROPIC_8X);
	_noteTex = LoadTexture("assets/note.png");
	SetTextureFilter(_noteTex, TEXTURE_FILTER_ANISOTROPIC_8X);
	_cursorTex = LoadTexture("assets/cursor.png");
	SetTextureFilter(_cursorTex, TEXTURE_FILTER_ANISOTROPIC_8X);
	_font = LoadFontEx("assets/nasalization.otf", 512, 0, 250);
	SetTextureFilter(_font.texture, TEXTURE_FILTER_ANISOTROPIC_8X);
	_background = _menuBackground;



	_loadingFade = 0;
	//load button tile set
	for(int y = 0; y < 3; y++)
	{
		for(int x = 0; x < 3; x++)
		{
			char str[40];
			strcpy(str, "assets/buttonTile_x.png");
			str[18] = '1' + x+y*3;
			_buttonTile[x][y] = LoadTexture(str);
		}
	}
}