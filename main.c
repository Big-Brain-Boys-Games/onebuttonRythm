#include <stdio.h>
//#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION

#include "include/miniaudio.h"
#include "include/raylib.h"


#define GLSL_VERSION            330


float * _pNotes;
FILE * _pFile;
void (*_pGameplayFunction)();
Texture2D _background, _heartTex, _healthBarTex, _noteTex, _cursorTex;
// Music music;
// Sound hitSE, missHitSE, missSE;
bool _noBackground = false;
float _fadeOut= 0;
float _health = 50;
int _noteIndex = 1, _amountNotes =0 ;
int _score= 0;
char * _pMap = "testMap";
float _musicTime = 0;

float _maxMargin = 0.1;
int _hitPoints = 5;
int _missPenalty = 10;
float _scrollSpeed = 0.6;
Color _fade = WHITE;

ma_decoder _decoder;
ma_device_config _deviceConfig;
ma_device _device;

bool _musicPlaying;


void fFail ();
void fCountDown ();
void fEndScreen ();
void fMainMenu();


int _musicFrameCount = 0;
int _musicLength = 0;
void* _pMusic;


void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	if(!_musicPlaying)
		return;

	if(_musicLength > _musicFrameCount)
		memcpy(pOutput, _pMusic+_musicFrameCount*sizeof(_Float32)*2, frameCount*sizeof(_Float32)*2);
	_musicFrameCount += frameCount;


    (void)pInput;
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

void saveFile (int noteIndex)
{
	fwrite(_pNotes, sizeof(float), noteIndex, _pFile);
	printf("written map data\n");
}

void loadMusic(char * file)
{
	_musicPlaying = false;
	if(_musicLength != 0)
	{
		//unload previous music
		free(_pMusic);
		ma_decoder_uninit(&_decoder);
	}
	ma_result result;

	result = ma_decoder_init_file(file, NULL, &_decoder);
    if (result != MA_SUCCESS) {
        printf("failed to open music file %s\n", file);
		exit(0);
    }
	printf("decoder format %i   Sizeof %i\n", _decoder.outputFormat, sizeof(_Float32));
	int lastFrame = -1;
	int musicSize = 10;
	ma_decoder_get_length_in_pcm_frames(&_decoder, &_musicLength);
	printf("Music length %i\n", _musicLength);
	_pMusic = calloc(sizeof(_Float32)*2, _musicLength);
	void * pCursor = _pMusic;
	int frameCounter = 0;
	while(_decoder.readPointerInPCMFrames !=lastFrame)
	{
		lastFrame = _decoder.readPointerInPCMFrames;
		ma_decoder_read_pcm_frames(&_decoder, pCursor, 256, &frameCounter);
		int size = 0;
		// printf("size %i \t pCursor %i\n", size, *(int*)(pCursor + size));
		pCursor += sizeof(_Float32)*2*256;
	}


	_deviceConfig = ma_device_config_init(ma_device_type_playback);
    _deviceConfig.playback.format   = _decoder.outputFormat;
    _deviceConfig.playback.channels = _decoder.outputChannels;
    _deviceConfig.sampleRate        = _decoder.outputSampleRate;
    _deviceConfig.dataCallback      = data_callback;
    _deviceConfig.pUserData         = &_decoder;
	
}

void startMusic()
{
	if(_musicFrameCount != 0)
	{
		//unload previous devices
		ma_device_uninit(&_device);
   		ma_decoder_uninit(&_decoder);
	}
	if (ma_device_init(NULL, &_deviceConfig, &_device) != MA_SUCCESS) {
			printf("Failed to open playback device.\n");
			ma_decoder_uninit(&_decoder);
			return;
	}

	if (ma_device_start(&_device) != MA_SUCCESS) {
		printf("Failed to start playback device.\n");
		ma_device_uninit(&_device);
		ma_decoder_uninit(&_decoder);
		return;
	}
	_musicPlaying = true;
}

void loadMap (int fileType)
{
	char * pStr = malloc(strlen(_pMap) + 12);
	strcpy(pStr, _pMap);
	strcat(pStr, "/image.png");
	_background = LoadTexture(pStr);
	strcpy(pStr, _pMap);
	strcat(pStr, "/music.mp3");

	// ma_result result
	// music = LoadMusicStream(pStr);
	loadMusic(pStr);
	

	strcpy(pStr, _pMap);
	strcat(pStr, "/map.data");
	if(fileType == 0)
	{
		_pFile = fopen(pStr, "rb");
		float size;
		fread(&size, 1, sizeof(float), _pFile);
		printf("size: %i", (int)size);
		if(_amountNotes != 0)
		{
			free(_pNotes);
		}
		_pNotes = calloc(size, sizeof(float));
		rewind (_pFile);
		fread(_pNotes, size, sizeof(float), _pFile);
		fclose(_pFile);
		_amountNotes = size;
	}else
	{
		_pFile = fopen(pStr, "wb");
		_pNotes = calloc(100000, sizeof(float));
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
	DrawTextureEx(_cursorTex, (Vector2){.x=GetMouseX(), .y=GetMouseY()}, 0, size, WHITE);
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

void drawButton(Rectangle rect, char * text)
{
	Color color = WHITE;
	if(mouseInRect(rect))
		color = LIGHTGRAY;
	if(mouseInRect(rect) && IsMouseButtonDown(0))
		color = GRAY;
	DrawRectangle(rect.x, rect.y, rect.width, rect.height, color);
	int textSize = MeasureText(text, GetScreenWidth() * 0.05);
	DrawText(text, rect.x + rect.width / 2 - textSize / 2, rect.y + GetScreenHeight() * 0.01, GetScreenWidth() * 0.05, (color.r == GRAY.r) ? BLACK : DARKGRAY);
}

void fRecording ()
{
	// UpdateMusicStream(music);
	BeginDrawing();
		ClearBackground(BLACK);
		if(!_noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)_background.width;
			DrawTextureEx(_background, (Vector2){.x=0, .y=(GetScreenHeight() - _background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}

		if(GetKeyPressed())
		{
			printf("keyPressed! \n");
			
			//todo fix
			_pNotes[_noteIndex] = 0;
			//GetMusicTimePlayed(music);
			
			// printf("written value %f  to index: %i, supposed to be %f\n", notes[noteIndex], noteIndex,  GetMusicTimePlayed(music));
			_noteIndex++;
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			ClearBackground(BLACK);
		}
		// DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=noLessThanZero(fadeOut - GetMusicTimePlayed(music))*255});

	EndDrawing();
}


void dNotes () 
{
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;
	if(_noteIndex < _amountNotes)//draw notes after line
	{
		for(int i = _noteIndex; i >= 0 && _pNotes[i] + _scrollSpeed > _musicTime; i--)
		{
			if(i < 0) continue;
			//DrawCircle( middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			float x = middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed) - _noteTex.width * scaleNotes / 2;
			DrawTextureEx(_noteTex, (Vector2){.x=x, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=128,.g=128,.b=128,.a= noLessThanZero(255-(_pNotes[i] - _musicTime) * (255/_scrollSpeed)) / 2});

		}

		//draw notes before line
		for(int i = _noteIndex; i < _amountNotes && _pNotes[i] - _scrollSpeed < _musicTime; i++)
		{
			//DrawCircle( middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(_noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			DrawTextureEx(_noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed) - _noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,WHITE);
		}
	}
	DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=255/2});
}

#define feedback(newFeedback) feedbackSayings[feedbackIndex] = newFeedback; feedbackIndex++; if(feedbackIndex > 4) feedbackIndex = 0;
void fPlaying ()
{
	static char *feedbackSayings [5];
	static int feedbackIndex = 0;
	_musicTime += GetFrameTime();

	// _pGameplayFunction = &fEndScreen;

	BeginDrawing();
		ClearBackground(BLACK);
		
		if(!_noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)_background.width;
			DrawTextureEx(_background, (Vector2){.x=0, .y=(GetScreenHeight() - _background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}
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

		
		//printf("note %f   time: %f\n", notes[noteIndex], musicTime);
		if(_noteIndex < _amountNotes && _musicTime - _maxMargin > _pNotes[_noteIndex])
		{
			//passed note
			_noteIndex++;
			_health -= _missPenalty;
			printf("missed note %f  index %i   note: %f\n", _musicTime, _noteIndex, _pNotes[_noteIndex]);
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			_fade = RED;
			feedback("miss!");
		}

		if(GetKeyPressed() && _noteIndex < _amountNotes)
		{
			//printf("keyPressed! \n");
			
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			_fade = WHITE;
			float closestTime = 55;
			int closestIndex = 0;
			for(int i = _noteIndex; i <= _noteIndex + 1 && i < _amountNotes; i++)
			{
				//printf("fabs: %f\n",fabs(_pNotes[i] - musicTime));
				if(closestTime > fabs(_pNotes[i] - _musicTime))
				{
					closestTime = fabs(_pNotes[i] - _musicTime);
					closestIndex = i;
				}
			}
			//printf("closestTime: %f\n", closestTime);
			if(closestTime < _maxMargin)
			{
				printf("hit note! %i\n", _noteIndex);
				while(_noteIndex < closestIndex)
				{
					_noteIndex++;
					_health -= _missPenalty;
					feedback("miss!");
				}
				int healthAdded = noLessThanZero(_hitPoints - closestTime * (_hitPoints / _maxMargin));
				_health += healthAdded;
				//feedback("hit!");
				int scoreAdded = noLessThanZero(300 - closestTime * (300 / _maxMargin));
				if(_score > 200) {
					feedback("300!");
				}else if (_score > 100) {
					feedback("200!");
				} else {
					feedback("100!");
				}
				_score += scoreAdded;
			
				_noteIndex++;
				printf("new note index %i\n", _noteIndex);
				// PlaySoundMulti(hitSE);
			}else
			{
				printf("missed note\n");
				feedback("miss!");
				_fade = RED;
				_health -= _missPenalty;
				// PlaySoundMulti(missHitSE);
			}
			ClearBackground(BLACK);
			printf("health %f \n", _health);
		}
		printf("health %f \n", _health);

		if(_health > 100)
			_health = 100;
		if(_health < 0)
			_health = 0;

		if(feedbackIndex >= 5)
			feedbackIndex = 0;

		float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
		float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
		//DrawTextureEx(heartTex, (Vector2){.x=0, .y=0}, 0, heartScale,WHITE);
		DrawTextureEx(_healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale,
			.y=GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
		DrawTextureEx(_heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale,WHITE);
		
		
		// DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=fade.r,.g=fade.g,.b=fade.b,.a=noLessThanZero(fadeOut - GetMusicTimePlayed(music))*255});

		if(_health <= 0)
		{
			printf("goto fail\n");
			//goto fFail
			_pGameplayFunction = &fFail;
			// StopMusicStream(music);
		}

	EndDrawing();
}

void fFail ()
{
	_musicPlaying = false;
	BeginDrawing();
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
		drawButton(playButton,"retry");

		Rectangle MMButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.85, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(MMButton,"main menu");
		
		

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
		drawCursor();

	EndDrawing();
}

void fEndScreen ()
{
	_musicPlaying = false;
	BeginDrawing();
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
		drawButton(playButton,"retry");

		Rectangle MMButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.85, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(MMButton,"main menu");
		
		

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
		drawCursor();

	EndDrawing();
}


void fCountDown ()
{
	_musicPlaying = false;
	static float countDown  = 0;
	if(countDown == 0) countDown = GetTime() + 3;
	// printf("countDown %f  getTime %f\n", countDown, GetTime());
	if(countDown - GetTime() < 0)
	{
		countDown = 0;
		//switching to playing map
		printf("switching to playing map! \n");
		
	_pGameplayFunction = &fPlaying;

		startMusic();
		
		
		// PlayMusicStream(music);  
		// SetMusicVolume(music, 0.8);
		_health = 50;
		_score = 0;
		_noteIndex =1;
		_musicTime = 0;
	}
	BeginDrawing();
		ClearBackground(BLACK);
		if(!_noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)_background.width;
			DrawTextureEx(_background, (Vector2){.x=0, .y=(GetScreenHeight() - _background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}
		//DrawFPS(500,0);

		//drawing parts of UI to not be too jarring when countdown is over

		//draw notes
		float width = GetScreenWidth() * 0.005;
		float middle = GetScreenWidth() /2;
		
		
		float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

		DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=255/2});

		//draw score
		char * tmpString = malloc(9);
		sprintf(tmpString, "%i", _score);
		DrawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
		
		drawCursor();
		
		float heartScale = (GetScreenWidth() * 0.08) / _heartTex.width;
		float healthBarScale = (GetScreenHeight() * 0.4) / _healthBarTex.height;
		//DrawTextureEx(heartTex, (Vector2){.x=0, .y=0}, 0, heartScale,WHITE);
		DrawTextureEx(_healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (_heartTex.width / 2) * heartScale - (_healthBarTex.width / 2) * healthBarScale,
			.y=GetScreenHeight() * 0.85 - _healthBarTex.height * healthBarScale + (_heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
		DrawTextureEx(_heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - _health / 250)}, 0, heartScale,WHITE);
		DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=0,.g=0,.b=0,.a=128});

		sprintf(tmpString, "%i", (int)(countDown - GetTime() + 1));
		float textSize = MeasureText(tmpString, GetScreenWidth() * 0.3);
		DrawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.3, GetScreenWidth() * 0.3, WHITE);

		free(tmpString);
	EndDrawing();
}


#define SUPPORT_FILEFORMAT_WAV
#define SUPPORT_FILEFORMAT_MP3



void fEditor ()
{
	static bool isPlaying = false;
	static float _scrollSpeed = 1;
	_musicPlaying = isPlaying;
	//printf("musicTime: %.2f \t rMusicTime: %.2f\n", _musicTime, (float)_musicFrameCount/(_decoder.outputSampleRate));
	if(isPlaying) {
		// UpdateMusicStreamCustom(music);
		_musicTime += GetFrameTime();
	}else
	{
		_musicFrameCount = _musicTime*_decoder.outputSampleRate;
		if(IsKeyDown(KEY_RIGHT) || GetMouseWheelMove() > 0) _musicTime+= GetFrameTime()*_scrollSpeed;
		if(IsKeyDown(KEY_LEFT) || GetMouseWheelMove() < 0) _musicTime-= GetFrameTime()*_scrollSpeed;
		if(IsKeyPressed(KEY_UP)) _scrollSpeed *= 1.2;
		if(IsKeyPressed(KEY_DOWN)) _scrollSpeed /= 1.2;
		if(_scrollSpeed == 0) _scrollSpeed = 0.01;
		printf("Mousewheel: %f  \n Key Right: %d", GetMouseWheelMove(), IsKeyDown(KEY_RIGHT));	
	}

	if(_musicTime < 0)
		_musicTime = 0;
	

	BeginDrawing();
		ClearBackground(BLACK);
		
		if(!_noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)_background.width;
			DrawTextureEx(_background, (Vector2){.x=0, .y=(GetScreenHeight() - _background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(_background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = _background.height, .width= _background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}
		//DrawFPS(500,0);

		//draw notes
		float width = GetScreenWidth() * 0.005;
		float middle = GetScreenWidth() /2;
		
		float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;

		dNotes();

		float closestTime = 55;
		int closestIndex = 0;
		for(int i = 0; i < _amountNotes; i++)
		{
			if(closestTime > fabs(_pNotes[i] - _musicTime))
			{
				closestTime = fabs(_pNotes[i] - _musicTime);
				closestIndex = i;
			}
		}
		if (IsKeyPressed(KEY_X))
		{
			if(closestTime < _maxMargin)
			{
			_amountNotes--;
			for(int i = closestIndex; i < _amountNotes; i++)
			{
				_pNotes[i] = _pNotes[i+1];
			}
			float * tmp = malloc(sizeof(float) * _amountNotes);
			for(int i = 0; i < _amountNotes; i++)
			{
				tmp[i] = _pNotes[i];
			}
			free(_pNotes);
			_pNotes = tmp;
			}
		}
		if(IsKeyPressed(KEY_Z))
		{
			_amountNotes++;
			float * tmp = calloc(_amountNotes, sizeof(float));
			for(int i = 0; i < _amountNotes-1; i++)
			{
				tmp[i] = _pNotes[i];
			}
			for(int i = closestIndex; i < _amountNotes - 1; i++)
			{
				tmp[i+1] = _pNotes[i];
			}
			
			
			free(_pNotes);
			_pNotes = tmp;
			_pNotes[closestIndex] = _musicTime;
		}

		if(IsKeyPressed(KEY_SPACE))
		{
			isPlaying = !isPlaying;

			if(isPlaying)
			{
				//sets music audio to right time
				// printf("before framesProcessed %i     %f\n", music.stream.buffer->framesProcessed, GetMusicTimePlayed(music));
				// printf("before frameCursorPos %i     %f\n", music.stream.buffer->frameCursorPos, GetMusicTimePlayed(music));
				// printf("%lu\n", music.stream.buffer);
				// music.stream.buffer->framesProcessed = musicTime * music.stream.sampleRate;
				// printf("music buffer: %d\n", music.stream.buffer);
				// printf("after framesProcessed %i     %f\n", music.stream.buffer->framesProcessed, GetMusicTimePlayed(music));
				// printf("after frameCursorPos %i     %f\n", music.stream.buffer->frameCursorPos, GetMusicTimePlayed(music));
			}


		}


		// if(GetKeyPressed() && noteIndex < _amountNotes)
		// {
		// 	//printf("keyPressed! \n");
			
		// 	fadeOut = GetMusicTimePlayed(music) + 0.1;
		// 	fade = WHITE;
		// 	float closestTime = 55;
		// 	int closestIndex = 0;
		// 	for(int i = noteIndex; i <= noteIndex + 1 && i < _amountNotes; i++)
		// 	{
		// 		//printf("fabs: %f\n",fabs(_pNotes[i] - musicTime));
		// 		if(closestTime > fabs(_pNotes[i] - musicTime))
		// 		{
		// 			closestTime = fabs(_pNotes[i] - musicTime);
		// 			closestIndex = i;
		// 		}
		// 	}
		// 	//printf("closestTime: %f\n", closestTime);
		// 	if(closestTime < maxMargin)
		// 	{
		// 		printf("hit note! %i\n", noteIndex);
		// 		while(noteIndex < closestIndex)
		// 		{
		// 			noteIndex++;
		// 			health -= missPenalty;
		// 		}
		// 		int healthAdded = noLessThanZero(hitPoints - closestTime * (hitPoints / maxMargin));
		// 		health += healthAdded;			
		// 		int scoreAdded = noLessThanZero(300 - closestTime * (300 / maxMargin));
		// 		score += scoreAdded;
			
		// 		noteIndex++;
		// 		printf("new note index %i\n", noteIndex);
		// 		PlaySoundMulti(hitSE);
		// 	}else
		// 	{
		// 		printf("missed note\n");			
		// 		fade = RED;
		// 		health -= missPenalty;
		// 		PlaySoundMulti(missHitSE);
		// 	}
		// 	ClearBackground(BLACK);
		// 	printf("health %f \n", health);
		// }

	EndDrawing();
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
		drawButton(playButton,"play");

		Rectangle editorButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.5, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(editorButton,"Editor");
		

		if(IsMouseButtonReleased(0) && mouseInRect(playButton))
		{
			//switching to playing map
			loadMap(0);
			printf("switching to playing map! \n");
			
		_pGameplayFunction = &fCountDown;
		}

		if(IsMouseButtonReleased(0) && mouseInRect(editorButton))
		{
			//switching to playing map
			loadMap(0);
			startMusic();
			// PlayMusicStream(music);  
			// SetMusicVolume(music, 0.8);
			_health = 50;
			_score = 0;
			_noteIndex =1;
			_musicTime = 0;
			printf("switching to editor map! \n");
			
		_pGameplayFunction = &fEditor;
		}

		drawCursor();

	EndDrawing();
}

int main (int argc, char **argv)
{
	SetTargetFPS(60);
	// printf("size of int: %i", sizeof(int));
	// printf("size of float: %i", sizeof(float));
	//if(argc == 3) limit = strtol(argv[2], &p, 10);
	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Simple rythm game");
	// InitAudioDevice();
	//rlDisableDepthTest();
	//rlDisableBackfaceCulling();
	//rlDisableScissorTest();
	HideCursor();
	
	_heartTex = LoadTexture("heart.png");
	_healthBarTex = LoadTexture("healthBar.png");
	_noteTex = LoadTexture("note.png");
	_cursorTex = LoadTexture("cursor.png");
	resetBackGround();
	
	// hitSE = LoadSound("hit.mp3");
	// SetSoundVolume(hitSE, 0.6f);
	// missHitSE = LoadSound("missHit.mp3");
	// SetSoundVolume(missHitSE, 1);
	// missSE = LoadSound("missSE.mp3");
	// SetSoundVolume(missSE, 1);
	
	

	
	// if(0)
	// {
	// 	file = fopen("testMap/map.data", "wb");
	// 	notes = calloc(100000, sizeof(float));
	// }else
	// {
	// 	file = fopen("testMap/map.data", "rb");
	// 	float size;
	// 	fread(&size, 1, sizeof(float), file);
	// 	printf("size: %i", (int)size);
	// 	notes = calloc(size, sizeof(float));
	// 	rewind (file);
	// 	fread(notes, size, sizeof(float), file);
	// }
	

	//Shader shad = LoadShader(0, "fragment.shader");
	//SetTargetFPS(20);
	Vector2 mousePos;
	

	// if(0)
	// {
	// 	
	_pGameplayFunction = &fRecording;
	// }else
	// {
	// 	
	_pGameplayFunction = &fPlaying;
	// }
	
	
	_pGameplayFunction = &fMainMenu;
	
	//GetMusicTimePlayed(music)/GetMusicTimeLength(music) < 0.99
	while (!WindowShouldClose()) {
		mousePos = GetMousePosition();
		
		//printf("music lenght %f   index %i\n", GetMusicTimePlayed(music)/GetMusicTimeLength(music), noteIndex);

		// if(IsKeyPressed(KEY_MINUS)) {limit /= 10; }
		// if(IsKeyPressed(KEY_EQUAL)) {limit *= 10; }
		//rentmp = renTex;
		//renTex = ren2Tex;
		//ren2Tex = rentmp;
		
		//ClearBackground(BLACK);
		//while((frameTime - clock()) / CLOCKS_PER_SEC < 0.05)

		(*_pGameplayFunction)();
		
		

	}
	// if(0 && GetMusicTimePlayed(music)/GetMusicTimeLength(music) >= 0.99)
	// {
	// 	notes[0] = noteIndex;
	// 	saveFile(noteIndex);
	// }

	UnloadTexture(_background);
	CloseWindow();
	return 0;

}
