#include <stdio.h>
//#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION

#include "include/miniaudio.h"
#include "include/raylib.h"


#define GLSL_VERSION            330

//Timestamp of all the notes
float * _pNotes;
FILE * _pFile;
void (*_pGameplayFunction)();
void (*_pNextGameplayFunction)();
Texture2D _background, _heartTex, _healthBarTex, _noteTex, _cursorTex;

bool _noBackground = false;
float _fadeOut= 0;
float _health = 50;
int _noteIndex = 1, _amountNotes =0 ;
int _score= 0;
char * _pMap;

//Where is the current audio
float _musicHead = 0;

float _maxMargin = 0.1;
int _hitPoints = 5;
int _missPenalty = 10;
float _scrollSpeed = 0.6;
Color _fade = WHITE;

ma_decoder _decoder;
ma_device_config _deviceConfig;
ma_device _device;

bool _musicPlaying;

Color _UIColor = WHITE;


void fFail ();
void fCountDown ();
void fEndScreen ();
void fMainMenu();
void fEditor ();


int _musicFrameCount = 0;
int _musicLength = 0;
int bpm = 150;

#define EFFECT_BUFFER_SIZE 48000*4*4
void* _pMusic;
void* _pEffectsBuffer;
int _effectOffset;

void* _pHitSE;
int _hitSE_Size;

void* _pMissHitSE;
int _missHitSE_Size;

void* _pMissSE;
int _missSE_Size;

void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{

	//music
	if(_musicPlaying)
	{
		if(_musicLength > _musicFrameCount)
			memcpy(pOutput, _pMusic+_musicFrameCount*sizeof(_Float32)*2, frameCount*sizeof(_Float32)*2);
		_musicFrameCount += frameCount;
	}
	//sound effects

	for(int i = 0; i < frameCount*2; i++)
	{
		((_Float32*)pOutput)[i] += ((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(48000*4)];
		((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(48000*4)] = 0;
	}
	_effectOffset+=frameCount*2;



    (void)pInput;
}

bool endOfMusic()
{
	if(_musicLength < _musicFrameCount)
		return true;
	return false;
}

void resetBackGround()
{
	_background = LoadTexture("background.png");
}

float noLessThanZero(float var)
{
	if(var < 0) return 0;
	return var;
}

void printAllNotes()
{
	for(int i = 0; i < _amountNotes; i++)
	{
		printf("note %.2f\n", _pNotes[i]);
	}
}


void saveFile (int noteAmount)
{
	printf("written map data\n");
	char * mapName = "Map1";
	char * Creator = "Me";
	int Difficulty = 3;
	fprintf(_pFile, "[Name]\n");
	fprintf(_pFile, "%s\n", mapName);
	fprintf(_pFile, "[Creator]\n");
	fprintf(_pFile, "%s\n", Creator);
	fprintf(_pFile, "[Difficulty]\n");
	fprintf(_pFile, "%i\n", Difficulty);
	fprintf(_pFile, "[BPM]\n");
	fprintf(_pFile, "%i\n", bpm);
	fprintf(_pFile, "[Notes]\n");
	for(int i = 0; i < noteAmount; i++)
	{
		if(_pNotes[i] == 0)
			continue;
		fprintf(_pFile, "%f\n", _pNotes[i]);
	}
	fclose(_pFile);
}

void * loadAudio(char * file, ma_decoder * decoder, int * audioLength)
{
	ma_result result;
	printf("loading sound effect %s\n", file);
	ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 2, 48000);
	decoder_config.resampling.linear.lpfOrder = MA_MAX_FILTER_ORDER;
	result = ma_decoder_init_file(file, &decoder_config, decoder);
    if (result != MA_SUCCESS) {
        printf("failed to open music file %s\n", file);
		exit(0);
    }
	int lastFrame = -1;
	ma_decoder_get_length_in_pcm_frames(decoder, audioLength);
	void * pAudio = calloc(sizeof(_Float32)*2*2, *audioLength); //added some patting to get around memory issue //todo fix this work around
	void * pCursor = pAudio;
	while(decoder->readPointerInPCMFrames !=lastFrame)
	{
		lastFrame = decoder->readPointerInPCMFrames;
		ma_decoder_read_pcm_frames(decoder, pCursor, 256, NULL);
		pCursor += sizeof(_Float32)*2*256;
	}
	ma_decoder_uninit(decoder);
	return pAudio;
}

void loadMusic(char * file)
{
	_musicPlaying = false;
	if(_musicLength != 0)
	{
		//unload previous music
		free(_pMusic);
	}
	_pMusic = loadAudio(file, &_decoder, &_musicLength);

	_deviceConfig = ma_device_config_init(ma_device_type_playback);
    _deviceConfig.playback.format   = _decoder.outputFormat;
    _deviceConfig.playback.channels = _decoder.outputChannels;
    _deviceConfig.sampleRate        = _decoder.outputSampleRate;
    _deviceConfig.dataCallback      = data_callback;
    _deviceConfig.pUserData         = &_decoder;
	// _deviceConfig.periodSizeInMilliseconds = 300;
}

float getMusicDuration() {
	return _musicLength / (float)_decoder.outputSampleRate;
}

float getMusicPosition() {
	return _musicFrameCount / (float)_decoder.outputSampleRate;
}

void fixMusicTime()
{
	if(fabs(_musicHead - getMusicPosition()) > 0.02)
		_musicHead = getMusicPosition();
}

int getBarsCount() {
	return bpm*getMusicDuration()/60/4;
}

int getBeatsCount() {
	return bpm*getMusicDuration()/60;
}

void playAudioEffect(void * effect, int size)
{
	for(int i = 0; i < size; i++)
	{
		((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(48000*4)] += ((_Float32*)effect)[i];
	}
}

void startMusic()
{
	if(_musicFrameCount != 0)
	{
		//unload previous devices
		ma_device_uninit(&_device);
	}
	if (ma_device_init(NULL, &_deviceConfig, &_device) != MA_SUCCESS) {
			printf("Failed to open playback device.\n");
			return;
	}

	if (ma_device_start(&_device) != MA_SUCCESS) {
		printf("Failed to start playback device.\n");
		ma_device_uninit(&_device);
		return;
	}
	_musicPlaying = true;
}

void stopMusic()
{
	ma_device_uninit(&_device);
	_musicFrameCount = 0;
	_musicHead = 0;
	_musicPlaying = false;
	_musicLength = 0;
}

enum FilePart{fpNone, fpName, fpCreator, fpDifficulty, fpBPM, fpNotes};

void loadMap (int fileType)
{
	char * pStr = malloc(strlen(_pMap) + 12);
	strcpy(pStr, _pMap);
	strcat(pStr, "/image.png");
	_background = LoadTexture(pStr);
	strcpy(pStr, _pMap);
	strcat(pStr, "/song.mp3");

	// ma_result result
	// music = LoadMusicStream(pStr);
	loadMusic(pStr);
	

	strcpy(pStr, _pMap);
	strcat(pStr, "/map.data");
	if(fileType == 0)
	{
		_pFile = fopen(pStr, "rb");

		//text file
		char line [1000];
		enum FilePart mode = fpNone;
		_amountNotes = 50;
		_noteIndex = 0;
		_pNotes = malloc(sizeof(float)*_amountNotes);
   		while(fgets(line,sizeof(line),_pFile)!= NULL)
		{
			int stringLenght = strlen(line);
			bool emptyLine = true;
			for(int i = 0; i < stringLenght; i++)
				if(line[i] != ' ' && line[i] != '\n')
					emptyLine = false;
			
			if(emptyLine)
				continue;

			if(strcmp(line, "[Name]\n") == 0)			{mode = fpName;			continue;}
			if(strcmp(line, "[Creator]\n") == 0)		{mode = fpCreator;		continue;}
			if(strcmp(line, "[Difficulty]\n") == 0)		{mode = fpDifficulty;	continue;}
			if(strcmp(line, "[BPM]\n") == 0)			{mode = fpBPM;			continue;}
			if(strcmp(line, "[Notes]\n") == 0)			{mode = fpNotes;		continue;}
			switch(mode)
			{
				case fpNone:
					break;
				case fpName:
					//todo save name
					break;
				case fpCreator:
					//todo save creator
					break;
				case fpDifficulty:
					//todo save difficulty
					break;
				case fpBPM:
					bpm = atoi(line);
					printf("set bpm: %i\n", bpm);	
					break;
				case fpNotes:
					
					if(_noteIndex <= _amountNotes)
					{
						_amountNotes += 50;
						_pNotes = realloc(_pNotes, _amountNotes);
					}
					_pNotes[_noteIndex] = atof(line);
					printf("note %i  %f\n", _noteIndex, _pNotes[_noteIndex]);
					_noteIndex++;
					break;
			}
		}
		_amountNotes = _noteIndex;
		_noteIndex = 0;
		fclose(_pFile);
	}else
	{
		_pFile = fopen(pStr, "wb");
	}
	free(pStr);

	if(_background.id == 0)
	{
		printf("no background texture found\n");
		_background = LoadTexture("background.png");
		_noBackground = 1;
	}
}

void drawCursor ()
{
	static float lastClick;
	if(IsMouseButtonDown(0))
	{
		lastClick = GetTime();
	}
	float size = (GetScreenWidth() * (0.06 - 0.1 * noLessThanZero(0.06 + (lastClick - GetTime())) )) / _cursorTex.width;
	float xOffset = _cursorTex.width*size*0.3;
	float yOffset = _cursorTex.height*size*0.2;
	DrawTextureEx(_cursorTex, (Vector2){.x=GetMouseX()-xOffset, .y=GetMouseY()-yOffset}, 0, size, WHITE);
}

bool mouseInRect (Rectangle rect)
{
	int x = GetMouseX();
	int y = GetMouseY();
	if(rect.x < x && rect.y < y && rect.x + rect.width > x && rect.y + rect.height > y)
	{
		return true;
	}
	return false;
}

float fDistance(float x1, float y1, float x2, float y2)
{
	float x = x1 - x2;
	float y = y1 - y2;
	return sqrtf(fabs(x*x+y*y));
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

float musicTimeToScreen(float musicTime)
{
	float middle = GetScreenWidth() /2;
	return middle + middle * (musicTime - _musicHead) * (1/_scrollSpeed);
}

float screenToMusicTime(float x)
{
	float middle = GetScreenWidth() /2;
	return (x-middle)/(middle*(1/_scrollSpeed))+_musicHead;
}
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
			isGrabbed = true;
			_musicHead = GetMouseX()/(float)GetScreenWidth()*getMusicDuration();
		}
		if(!IsMouseButtonDown(0))
			isGrabbed = false;
	}
}

void drawBars()
{
	//Draw the bars
	float middle = GetScreenWidth()/2;
	float distBetweenBeats = getMusicDuration() / getBeatsCount();

	float distBetweenBars = distBetweenBeats*4;
	for (int i = screenToMusicTime(0)/distBetweenBars; i < screenToMusicTime(GetScreenWidth())/distBetweenBars; i++)
	{
		DrawRectangle(musicTimeToScreen(distBetweenBars*i),GetScreenHeight()*0.7,GetScreenWidth()*0.01,GetScreenHeight()*0.2,(Color){.r=_UIColor.r,.g=_UIColor.g,.b=_UIColor.b,.a=180});
	}

	for (int i = screenToMusicTime(0)/distBetweenBeats; i < screenToMusicTime(GetScreenWidth())/distBetweenBeats; i++)
	{
		if(i % 4 == 0) continue;
		DrawRectangle(musicTimeToScreen(distBetweenBeats*i),GetScreenHeight()*0.8,GetScreenWidth()*0.01,GetScreenHeight()*0.1,(Color){.r=_UIColor.r,.g=_UIColor.g,.b=_UIColor.b,.a=180});
	}
}

void drawVignette()
{
	DrawRectangleGradientV(0, 0, GetScreenWidth(), GetScreenHeight()*0.3, ColorAlpha(BLACK, 0.7), ColorAlpha(BLACK, 0));
	DrawRectangleGradientV(0, GetScreenHeight()*0.7, GetScreenWidth(), GetScreenHeight()*0.3, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.7));
}

void removeNote(int index)
{
	_amountNotes--;
	float * tmp = malloc(sizeof(float) * _amountNotes);
	memcpy(tmp, _pNotes, sizeof(float)*_amountNotes);
	for(int i = index; i < _amountNotes; i++)
	{
		tmp[i] = _pNotes[i+1];
	}
	free(_pNotes);
	_pNotes = tmp;
}

void newNote(float time)
{
	int closestIndex=0;
	_amountNotes++;
	float * tmp = calloc(_amountNotes, sizeof(float));
	for(int i = 0; i < _amountNotes-1; i++)
	{
		tmp[i] = _pNotes[i];
		if(tmp[i] < time)
		{
			closestIndex=i+1;
		}
	}
	for(int i = closestIndex; i < _amountNotes-1; i++)
	{
		tmp[i+1] = _pNotes[i];
	}
	
	
	free(_pNotes);
	_pNotes = tmp;
	_pNotes[closestIndex] = time;
}

void fRecording ()
{
	_musicHead += GetFrameTime();
	fixMusicTime();

	// UpdateMusicStream(music);
	BeginDrawing();
		ClearBackground(BLACK);
		drawBackground();

		if(GetKeyPressed() && _musicHead!=0)
		{
			newNote(_musicHead);
			ClearBackground(BLACK);
		}
		drawVignette();
		drawBars();
		drawProgressBar();
	EndDrawing();
	if(endOfMusic())
	{
		saveFile(_amountNotes);
		_pGameplayFunction = &fMainMenu;
	}
}

float noteFadeOut(float note)
{
	return fmin(1, fmax(0, (1-fabs(note - _musicHead) * (1/_scrollSpeed))*3));
}

void dNotes () 
{
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;
	
	if(_noteIndex < _amountNotes) //draw notes after line
	{
		for(int i = _noteIndex; i >= 0 && _pNotes[i] + _scrollSpeed > _musicHead; i--)
		{
			if(i < 0) continue;
			//DrawCircle( middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			DrawTextureEx(_noteTex, (Vector2){.x=musicTimeToScreen(_pNotes[i])- _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=128,.g=128,.b=128,.a=255*noteFadeOut(_pNotes[i])});

		}

	
		//draw notes before line
		for(int i = _noteIndex; i < _amountNotes && _pNotes[i] - _scrollSpeed < _musicHead; i++)
		{
			//DrawCircle( middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(_noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicHead) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			DrawTextureEx(_noteTex, (Vector2){.x=musicTimeToScreen(_pNotes[i])- _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=255,.g=255,.b=255,.a=255*noteFadeOut(_pNotes[i])});

		}
	}
	DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=_UIColor.r,.g=_UIColor.g,.b=_UIColor.b,.a=255/2});
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

#define feedback(newFeedback) feedbackSayings[feedbackIndex] = newFeedback; feedbackIndex++; if(feedbackIndex > 4) feedbackIndex = 0;
void fPlaying ()
{
	static char *feedbackSayings [5];
	static int feedbackIndex = 0;
	_musicHead += GetFrameTime();

	fixMusicTime();

	if(endOfMusic())
	{
		stopMusic();
		_pGameplayFunction = &fEndScreen;
		return;
	}

	BeginDrawing();
		ClearBackground(BLACK);
		
		drawBackground();
		//DrawFPS(500,0);

		//draw notes
		float width = GetScreenWidth() * 0.005;
		float middle = GetScreenWidth() /2;
		
		float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

		//draw score
		char * tmpString = malloc(9);
		sprintf(tmpString, "%i", _score);
		DrawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
		free(tmpString);

		dNotes();

		//draw feedback (300! 200! miss!)
		for(int i = 0, j = feedbackIndex-1; i < 5; i++, j--)
		{
			if(j < 0) j = 4;
			DrawText(feedbackSayings[j], GetScreenWidth() * 0.35, GetScreenHeight() * (0.6 + i * 0.1), GetScreenWidth() * 0.05, (Color){.r=255,.g=255,.b=255,.a=noLessThanZero(150 - i * 40)});
		}

		if(_noteIndex < _amountNotes && _musicHead - _maxMargin > _pNotes[_noteIndex])
		{
			//passed note
			_noteIndex++;
			_health -= _missPenalty;
			_fade = RED;
			feedback("miss!");
		}

		if(GetKeyPressed() && _noteIndex < _amountNotes)
		{
			_fade = WHITE;
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
					_health -= _missPenalty;
					feedback("miss!");
				}
				int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / _maxMargin));
				_health += healthAdded;
				int scoreAdded = noLessThanZero(300 - closestTime * (300 / _maxMargin));
				if(scoreAdded > 200) {
					feedback("300!");
				}else if (scoreAdded > 100) {
					feedback("200!");
				} else {
					feedback("100!");
				}
				_score += scoreAdded;
				_noteIndex++;
				playAudioEffect(_pHitSE, _hitSE_Size);
			}else
			{
				printf("missed note\n");
				feedback("miss!");
				_fade = RED;
				_health -= _missPenalty;
				playAudioEffect(_pMissHitSE, _missHitSE_Size);
			}
			ClearBackground(BLACK);
		}

		if(_health > 100)
			_health = 100;
		if(_health < 0)
			_health = 0;

		if(feedbackIndex >= 5)
			feedbackIndex = 0;

		float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
		float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
		DrawTextureEx(_healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale,
			.y=GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
		DrawTextureEx(_heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale,WHITE);

		if(_health <= 0)
		{
			printf("goto fail\n");
			//goto fFail
			stopMusic();
			_pGameplayFunction = &fFail;
		}
		drawVignette();
		drawProgressBar();
	EndDrawing();
}

void fFail ()
{
	_musicPlaying = false;
	BeginDrawing();
		ClearBackground(BLACK);
		drawBackground();
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
			//retrying map
			printf("retrying map!\n");
			_pGameplayFunction = &fCountDown;
		}
		if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
		{
			//retrying map
			printf("going to main Menu!\n");
			
			_pGameplayFunction = &fMainMenu;
			resetBackGround();
		}
		drawVignette();
		drawCursor();

	EndDrawing();
}

void fEndScreen ()
{
	_musicPlaying = false;
	BeginDrawing();
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
			//retrying map
			printf("retrying map! \n");
			
		_pGameplayFunction = &fCountDown;
		}
		if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
		{
			//retrying map
			printf("going to main Menu! \n");
			
		_pGameplayFunction = &fMainMenu;
			resetBackGround();
		}
		drawVignette();
		drawCursor();

	EndDrawing();
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
	_musicHead = GetTime()-countDown;

	BeginDrawing();
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
	EndDrawing();
}

struct Map{
	char * folder;
	char * name;
	char * creator;
	int difficulty;
	int bpm;
	Texture2D image;
};
typedef struct Map Map;

void drawMapThumbnail(Rectangle rect, Map map)
{
	float imageRatio = 0.7;
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonDown(0))
		color = GRAY;

	DrawRectangle(rect.x, rect.y, rect.width, rect.height, color);

	DrawRectangle(rect.x, rect.y, rect.width, rect.height*imageRatio, BLACK);

	//todo, proper scaling of thumbnails
	// float imageScaling = rect.width/map.image.width;
	// float imageOffset = rect.width - map.image.width*imageScaling/2;
	DrawTexturePro(map.image, (Rectangle){.x=0, .y=0, .width=map.image.width, .height=map.image.height}, (Rectangle){.x=rect.x, .y=rect.y, .width=rect.width, .height=rect.height*imageRatio},
			(Vector2){.x=0,.y=0}, 0, color);
	
	char text [100];
	sprintf(text, "%s - %s", map.name, map.creator);
	int textSize = MeasureText(text, GetScreenWidth() * 0.05);
	DrawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + GetScreenHeight() * 0.01+rect.height*imageRatio, GetScreenWidth() * 0.04, BLACK);
}

Map loadMapInfo(char * file)
{
	Map map;
	map.folder = malloc(100);
	strcpy(map.folder, file);
	char * pStr = malloc(strlen(file) + 12);
	strcpy(pStr, file);
	strcat(pStr, "/image.png");
	map.image = LoadTexture(pStr);
	
	strcpy(pStr, file);
	strcat(pStr, "/map.data");
	FILE * f;
	f = fopen(pStr, "rb");

	//text file
	char line [1000];
	enum FilePart mode = fpNone;
	while(fgets(line,sizeof(line),f)!= NULL)
	{
		int stringLenght = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLenght; i++)
			if(line[i] != ' ' && line[i] != '\n')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		if(strcmp(line, "[Name]\n") == 0)			{mode = fpName;			continue;}
		if(strcmp(line, "[Creator]\n") == 0)		{mode = fpCreator;		continue;}
		if(strcmp(line, "[Difficulty]\n") == 0)		{mode = fpDifficulty;	continue;}
		if(strcmp(line, "[BPM]\n") == 0)			{mode = fpBPM;	continue;}
		if(strcmp(line, "[Notes]\n") == 0)			{mode = fpNotes;		continue;}
		for(int i = 0; i < 100; i++)
					if(line[i] == '\n') line[i]= '\0';
		switch(mode)
		{
			case fpNone:
				break;
			case fpName:
				map.name = malloc(100);
				strcpy(map.name, line);
				break;
			case fpCreator:
				map.creator = malloc(100);
				strcpy(map.creator, line);
				//todo save creator
				break;
			case fpDifficulty:
				map.difficulty = atoi(line);
				//todo save difficulty
				break;
			case fpBPM:
				map.bpm = atoi(line);
				break;
			case fpNotes:
				//neat, notes :P
				break;
		}
	}
	fclose(f);
	free(pStr);

	if(map.image.id == 0)
	{
		printf("no background texture found\n");
		map.image = LoadTexture("background.png");
	}
	return map;
}

//todo add support for more maps
Map _pMaps [100];
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
			char file [100];
			strcpy(file, "maps/");
			strcat(file, files[i]);
			_pMaps[mapIndex] = loadMapInfo(&(file[0]));
			mapIndex++;
		}
		amount = mapIndex;
	}

	_musicPlaying = false;
	BeginDrawing();
		ClearBackground(BLACK);
		DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);

		int middle = GetScreenWidth()/2;

		Rectangle backButton = (Rectangle){.x=GetScreenWidth()*0.05, .y=GetScreenHeight()*0.05, .width=GetScreenWidth()*0.1, .height=GetScreenHeight()*0.05};
		drawButton(backButton, "back", 0.02);

		if(mouseInRect(backButton) && IsMouseButtonDown(0))
		{
			EndDrawing();
			_pGameplayFunction=&fMainMenu;
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
			drawMapThumbnail(mapButton,_pMaps[i]);

			if(IsMouseButtonReleased(0) && mouseInRect(mapButton))
			{
				_pMap = malloc(100);
				strcpy(_pMap, _pMaps[i].folder);
				//switching to playing map
				if(_pNextGameplayFunction != &fRecording)
					loadMap(0);
				else
					loadMap(1);
				_pGameplayFunction = _pNextGameplayFunction;
				
				
				if(_pNextGameplayFunction == &fPlaying || _pNextGameplayFunction == &fRecording)
					_pGameplayFunction = &fCountDown;
				
				if(_pGameplayFunction == &fEditor)
					startMusic();
				printf("selected map!\n");
				
			}
		}
		drawCursor();
	EndDrawing();
}

void fEditor ()
{
	static bool isPlaying = false;
	_musicPlaying = isPlaying;
	if(isPlaying) {
		_musicHead += GetFrameTime();
	}else
	{
		_musicFrameCount = _musicHead*_decoder.outputSampleRate;
		if(IsKeyDown(KEY_RIGHT) || GetMouseWheelMove() > 0) _musicHead+= GetFrameTime()*_scrollSpeed;
		if(IsKeyDown(KEY_LEFT) || GetMouseWheelMove() < 0) _musicHead-= GetFrameTime()*_scrollSpeed;
	}
	if(IsKeyPressed(KEY_UP)) _scrollSpeed *= 1.2;
	if(IsKeyPressed(KEY_DOWN)) _scrollSpeed /= 1.2;
	if(_scrollSpeed == 0) _scrollSpeed = 0.01;
	if(_musicHead < 0)
		_musicHead = 0;

	if(_musicHead > getMusicDuration())
		_musicHead = getMusicDuration();

	BeginDrawing();
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
			if (IsKeyPressed(KEY_X))
			{
				
				if(closestTime < _maxMargin)
				{
					removeNote(closestIndex);
				}
				
			}
			if(IsKeyPressed(KEY_Z) && fabs(closestTime) > 0.1f)
			{
				newNote(_musicHead);
			}

			if(IsKeyPressed(KEY_SPACE))
			{
				isPlaying = !isPlaying;
			}
		}
		drawVignette();

		drawCursor();
		drawBars();
		drawProgressBarI(true);
	EndDrawing();
	if(endOfMusic())
	{
		loadMap(1);
		saveFile(_amountNotes);
		_pGameplayFunction = &fMainMenu;

	}
}

void fMainMenu()
{
	
	_musicPlaying = false;
	BeginDrawing();
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
			//switching to playing map
			printf("switching to playing map!\n");
			
			_pNextGameplayFunction = &fPlaying;
			_pGameplayFunction = &fMapSelect;
		}

		if(IsMouseButtonReleased(0) && mouseInRect(editorButton))
		{
			//switching to editing map
			_health = 50;
			_score = 0;
			_noteIndex =0;
			_amountNotes = 0;
			_musicHead = 0;
			printf("switching to editor map!\n");
			
			_pNextGameplayFunction = &fEditor;
			_pGameplayFunction = &fMapSelect;
		}

		if(IsMouseButtonReleased(0) && mouseInRect(recordingButton))
		{
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
		}

		drawCursor();

	EndDrawing();
}

int main (int argc, char **argv)
{
	SetTargetFPS(60);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Simple rythm game");


	HideCursor();
	
	_heartTex = LoadTexture("heart.png");
	_healthBarTex = LoadTexture("healthBar.png");
	_noteTex = LoadTexture("note.png");
	_cursorTex = LoadTexture("cursor.png");
	resetBackGround();
	
	Vector2 mousePos;

	//todo do this smarter
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); //4 second long buffer
	ma_decoder tmp;
	_pHitSE = loadAudio("hit.mp3", &tmp, &_hitSE_Size);
	_pMissHitSE = loadAudio("missHit.mp3", &tmp, &_missHitSE_Size);
	_pMissSE = loadAudio("missHit.mp3", &tmp, &_missSE_Size);
	
	_pGameplayFunction = &fMainMenu;

	while (!WindowShouldClose()) {
		mousePos = GetMousePosition();

		(*_pGameplayFunction)();
		
	}
	UnloadTexture(_background);
	CloseWindow();
	return 0;

}
