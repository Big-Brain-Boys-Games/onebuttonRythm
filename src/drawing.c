
#include "drawing.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../deps/raylib/src/raylib.h"
Texture2D _cursorTex;
Texture2D _noteTex;
Texture2D _healthBarTex;
Texture2D _heartTex;
Texture2D _background, _menuBackground;
Texture2D _buttonTile[3][3];

Font _font;

Color _UIColor = WHITE;

Color _fade = WHITE;

extern float _musicHead, _scrollSpeed, *_pNotes;
extern int _noteIndex, _amountNotes, _clickPressSE_Size, _clickReleaseSE_Size;
extern bool _noBackground, _isKeyPressed;
extern void ** _pMusic, *_pClickPress, *_pClickRelease;
extern float _transition, _loadingFade;
extern Map * _map;
extern int _barMeasureCount;


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
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, GetFrameTime()*5));
	else
		DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, fmax(2-_transition, 0)));
}

float musicTimeToScreen(float musicTime)
{
	float middle = GetScreenWidth() / 2;
	return middle + middle * (musicTime - getMusicHead()) * (1 / _scrollSpeed);
}

float screenToMusicTime(float x)
{
	float middle = GetScreenWidth() / 2;
	return (x - middle) / (middle * (1 / _scrollSpeed)) + getMusicHead();
}

float noteFadeOut(float note)
{
	return fmin(1, fmax(0, (1-fabs(note - getMusicHead()) * (1/_scrollSpeed))*3));
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
	static float fade = 0;
	float width = GetScreenWidth() * 0.01;
	float middle = GetScreenWidth() /2;
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;
	

	DrawRectangleGradientH(0,0 , middle - width / 2, GetScreenHeight(), ColorAlpha(BLACK, 0.6), ColorAlpha(BLACK, 0.3));
	for(int i = _noteIndex; i >= 0 && _pNotes[i] + _scrollSpeed > getMusicHead(); i--)
	{
		if(i < 0) continue;
		//DrawCircle( middle + middle * (_pNotes[i] - getMusicHead()) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
		//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - getMusicHead()) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
		DrawTextureEx(_noteTex, (Vector2){.x=musicTimeToScreen(_pNotes[i])- _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=128,.g=128,.b=128,.a=255*noteFadeOut(_pNotes[i])});

	}

	if(_noteIndex < _amountNotes) //draw notes after line
	{
		//draw notes before line
		for(int i = _noteIndex; i < _amountNotes && _pNotes[i] - _scrollSpeed < getMusicHead(); i++)
		{
			//DrawCircle( middle + middle * (_pNotes[i] - getMusicHead()) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(_noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - getMusicHead()) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			DrawTextureEx(_noteTex, (Vector2){.x=musicTimeToScreen(_pNotes[i])- _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=255,.g=255,.b=255,.a=255*noteFadeOut(_pNotes[i])});

		}
	}
	DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), ColorAlpha(WHITE, 0.5*fade));
	if(_isKeyPressed)
		fade = 1;
	fade -= GetFrameTime()*10;
}

void drawMapThumbnail(Rectangle rect, Map *map, int highScore, int combo, float accuracy, bool selected)
{
	if(map->image.id == 0)
	{
		//load map image onto gpu(cant be loaded in sperate thread because opengl >:( )
		if(map->cpuImage.width != 0)
		{
			map->image = LoadTextureFromImage(map->cpuImage);
		}
		if(map->image.id == 0)
			map->image.id = -1; 
		else
			SetTextureFilter(map->image, TEXTURE_FILTER_BILINEAR);
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
	sprintf(text, "%s - %s", map->name, map->creator);
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
	int textSize = measureText(textPointer, GetScreenWidth() * 0.04);
	drawText(textPointer, rect.x + rect.width/2 - textSize / 2, rect.y + GetScreenHeight() * 0.01+rect.height*imageRatio, GetScreenWidth() * 0.04, DARKGRAY);
	
	sprintf(text, "%i", map->difficulty);
	DrawRectangle(rect.x + rect.width*0.13, rect.y + 0.60*rect.height, rect.width*0.2, rect.height*0.20, ColorAlpha(BLACK, 0.4));
	drawText(text, rect.x + rect.width*0.08, rect.y + 0.60*rect.height, GetScreenWidth() * 0.035, WHITE);
	sprintf(text, "%i:%i", (int)floorf(map->musicLength/60), (int)floorf(map->musicLength-floorf(map->musicLength/60)*60));
	drawText(text, rect.x + rect.width*0.06, rect.y + 0.68*rect.height, GetScreenWidth() * 0.035, WHITE);

	sprintf(text, "%i", highScore);
	if(highScore !=0)
	{
		DrawRectangle(rect.x + rect.width*0.78, rect.y + 0.02*rect.height, rect.width*0.2, rect.height*0.16, ColorAlpha(BLACK, 0.4));
		drawText(text, rect.x + rect.width*0.80, rect.y + 0.02*rect.height, GetScreenWidth() * 0.02, WHITE);
		sprintf(text, "%i", combo);
		drawText(text, rect.x + rect.width*0.82, rect.y + 0.08*rect.height, GetScreenWidth() * 0.02, WHITE);
		sprintf(text, "%.2f", accuracy);
		drawText(text, rect.x + rect.width*0.84, rect.y + 0.08*rect.height, GetScreenWidth() * 0.02, WHITE);
	}
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
	_background = _menuBackground;
}

void drawBars()
{
	//Draw the bars
	float middle = GetScreenWidth()/2;
	float distBetweenBeats = getMusicDuration() / getBeatsCount() / _barMeasureCount;

	float distBetweenBars = distBetweenBeats*4;
	for (int i = (screenToMusicTime(0)-_map->offset/1000.0)/distBetweenBars; i < (screenToMusicTime(GetScreenWidth())-_map->offset/1000.0)/distBetweenBars; i++)
	{
		DrawRectangle(musicTimeToScreen(distBetweenBars*i+_map->offset/1000.0),GetScreenHeight()*0.6,GetScreenWidth()*0.01,GetScreenHeight()*0.3,(Color){.r=255,.g=255,.b=255,.a=180});
	}

	for (int i = (screenToMusicTime(0)-_map->offset/1000.0)/distBetweenBeats; i < (screenToMusicTime(GetScreenWidth())-_map->offset/1000.0)/distBetweenBeats; i++)
	{
		if(i % 4 == 0) continue;
		DrawRectangle(musicTimeToScreen(distBetweenBeats*i+_map->offset/1000.0),GetScreenHeight()*0.7,GetScreenWidth()*0.01,GetScreenHeight()*0.2,(Color){.r=255,.g=255,.b=255,.a=180});
	}
}

void drawMusicGraph(float transparent)
{
	if(_pMusic == 0 || *_pMusic == 0)
		return;
	
	//music stuff
	float beginning = screenToMusicTime(0);
	float end = screenToMusicTime(GetScreenWidth());
	int amountBars = GetScreenWidth()/2;
	float timePerBar = (end-beginning)/amountBars;
	int samplesPerBar = getSamplePosition(timePerBar);
	int sampleBegin = getSamplePosition(beginning);

	//printf("beginning: %f \tend: %f\tamountBars: %i\ttimePerBar: %f\tsamplesPerBar: %i\tsampleBegin: %i\n",beginning,end,amountBars,timePerBar,samplesPerBar,sampleBegin);
	//drawing stuff
	int pixelsPerBar = GetScreenWidth()/amountBars;
	float scaleBar = GetScreenHeight()*0.2;
	//looping through all the bars / samples
	for(int i = 0; i < amountBars; i++)
	{
		float highest = 0;
		int sampleHead = sampleBegin + samplesPerBar*i;
		for(int j = 0; j < samplesPerBar; j++)
			if(sampleHead+j > 0 && fabs(((float*)*_pMusic)[sampleHead+j]) > highest)
				highest = fabs(((float*)*_pMusic)[sampleHead+j]);

		DrawRectangle(i*pixelsPerBar, GetScreenHeight()-highest*scaleBar, pixelsPerBar, highest*scaleBar, ColorAlpha(WHITE, transparent));
	}

}

void drawVignette()
{
	DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight()*0.3, ColorAlpha(BLACK, 0.5), ColorAlpha(BLACK, 0));
	DrawRectangleGradientV(0, GetScreenHeight()*0.7, GetScreenWidth(), GetScreenHeight()*0.3, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.5));
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
	drawText(str, GetScreenWidth()*0.85, GetScreenHeight()*0.85, GetScreenWidth()*0.03,_UIColor);
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


void drawBox(Rectangle rect, Color color)
{
	float size = GetScreenWidth()*0.02;
	while(rect.height < size*2)
		size *=0.5;

	int sectionsX = rect.width/size;
	int sectionsY = rect.height/size;
	float secSizeX = rect.width/sectionsX;
	float secSizeY = rect.height/sectionsY;

	//it's not done in a loop/ fully tiled because mid sections can be stretched making for better fits
	//corners
	Rectangle dest = (Rectangle){.x=rect.x, .y=rect.y, .width=secSizeX, .height=secSizeY};
	Rectangle source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[0][0], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x+secSizeX*(sectionsX-1), .y=rect.y, .width=secSizeX, .height=secSizeY};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[2][0], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x, .y=rect.y+secSizeY*(sectionsY-1), .width=secSizeX, .height=secSizeY};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[0][2], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x+secSizeX*(sectionsX-1), .y=rect.y+secSizeY*(sectionsY-1), .width=secSizeX, .height=secSizeY};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[2][2], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	//edges

	dest = (Rectangle){.x=rect.x+secSizeX, .y=rect.y, .width=secSizeX*(sectionsX-2), .height=secSizeY};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[1][0], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	dest = (Rectangle){.x=rect.x, .y=rect.y+secSizeY, .width=secSizeX, .height=secSizeY*(sectionsY-2)};
	source = (Rectangle){.x=0, .y=0, .width=_buttonTile[0][0].width, .height=_buttonTile[0][0].height};
	DrawTexturePro(_buttonTile[0][1], source, dest, (Vector2){.x=0, .y=0}, 0, color);

	//bottom
	dest = (Rectangle){.x=rect.x+secSizeX, .y=rect.y+secSizeY*(sectionsY-1), .width=secSizeX*(sectionsX-2), .height=secSizeY};
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

	// printf("sections %i %i  \tsecSize: %.2f %.2f\n", sectionsX, sectionsY, secSizeX, secSizeY);
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
	// 		// printf("dest %i %i\t %i %i\n", x, y, sX, sY);
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
	int screenSize = GetScreenWidth() > GetScreenHeight() ? GetScreenHeight() : GetScreenWidth();
	int textSize = measureText(text, screenSize * fontScale);
	drawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + rect.height*0.2, screenSize * fontScale, (color.r == GRAY.r) ? BLACK : DARKGRAY);
}

void drawLoadScreen()
{
	static float angle = 0;
	angle += GetFrameTime()*(sinf(GetTime()*4)+1.5)*300;
	DrawTextureTiled(_menuBackground, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
		(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, ColorAlpha(WHITE, _loadingFade));
	DrawRing((Vector2){.x=GetScreenWidth()/2, .y=GetScreenHeight()/2}, GetScreenWidth()*0.1, GetScreenWidth()*0.15, angle, angle+170, 50, ColorAlpha(WHITE, _loadingFade));
	if(_loadingFade== 1)
		drawVignette();
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
	int screenSize = GetScreenWidth() > GetScreenHeight() ? GetScreenHeight() : GetScreenWidth();
	int textSize = measureText(text, screenSize * fontScale);
	drawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + rect.height*0.2, screenSize * fontScale, (color.r == GRAY.r) ? BLACK : DARKGRAY);
}

void initDrawing()
{
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "One Button Rythm");
	SetTargetFPS(120);
	SetExitKey(0);

	HideCursor();

	_heartTex = LoadTexture("assets/heart.png");
	SetTextureFilter(_heartTex, TEXTURE_FILTER_BILINEAR);
	_healthBarTex = LoadTexture("assets/healthBar.png");
	SetTextureFilter(_healthBarTex, TEXTURE_FILTER_BILINEAR);
	_noteTex = LoadTexture("assets/note.png");
	SetTextureFilter(_noteTex, TEXTURE_FILTER_BILINEAR);
	_cursorTex = LoadTexture("assets/cursor.png");
	SetTextureFilter(_cursorTex, TEXTURE_FILTER_BILINEAR);
	_menuBackground = LoadTexture("assets/background.png");
	SetTextureFilter(_menuBackground, TEXTURE_FILTER_BILINEAR);
	_font = LoadFontEx("assets/nasalization.otf", 1024, 0, 250);
	SetTextureFilter(_font.texture, TEXTURE_FILTER_BILINEAR);
	_background = _menuBackground;

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