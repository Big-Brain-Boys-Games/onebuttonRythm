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
		((_Float32*)pOutput)[i] += ((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(4800*4)];
		((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(4800*4)] = 0;
	}
	_effectOffset+=frameCount;



    (void)pInput;
}

bool endOfMusic()
{
	if(_musicLength/20 < _musicFrameCount)
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
	fprintf(_pFile, "[Notes]\n");
	for(int i = 0; i < noteAmount; i++)
	{
		fprintf(_pFile, "%f\n", _pNotes[i]);
	}
	fclose(_pFile);
	//fwrite(_pNotes, sizeof(float), noteIndex, _pFile);
	
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
	printf("ma_resample_algorithm %i\n", decoder_config.resampling.algorithm);
	printf("decoder format %i   Sizeof %i\n", decoder->outputFormat, sizeof(_Float32));
	int lastFrame = -1;
	ma_decoder_get_length_in_pcm_frames(decoder, audioLength);
	printf("audio length %i\n", *audioLength);
	printf("audio samplerate: %i\n", decoder->outputSampleRate);
	void * pAudio = calloc(sizeof(_Float32)*2*2, *audioLength); //added some patting to get around memory issue //todo fix this work around
	void * pCursor = pAudio;
	printf("doing resampling %i\n", decoder->converter.hasResampler);
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

	printf("yoo %i\n", _decoder.outputSampleRate);

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
	if(fabs(_musicTime - getMusicPosition()) > 0.1)
		_musicTime = getMusicPosition();
}

void playAudioEffect(void * effect, int size)
{
	for(int i = 0; i < size; i++)
	{
		((_Float32*)_pEffectsBuffer)[(i+_effectOffset)%(4800*4)] += ((_Float32*)effect)[i];
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

enum FilePart{fpNone, fpName, fpCreator, fpDifficulty, fpNotes};

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
		// float size;
		// //fread(&size, 1, sizeof(float), _pFile);
		// printf("size: %i", (int)size);
		// if(_amountNotes != 0)
		// {
		// 	free(_pNotes);
		// }
		// _pNotes = calloc(size, sizeof(float));
		// rewind (_pFile);
		// fread(_pNotes, size, sizeof(float), _pFile);
		// fclose(_pFile);
		// _amountNotes = size;

		//text file
		char line [1000];
		enum FilePart mode = fpNone;
		_amountNotes = 50;
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
			if(strcmp(line, "[Difficulty]\n") == 0)	{mode = fpDifficulty;	continue;}
			if(strcmp(line, "[Notes]\n") == 0)		{mode = fpNotes;		continue;}
			printf("%i mode, %s\n", mode, line);
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
				case fpNotes:
					_noteIndex++;
					if(_noteIndex <= _amountNotes)
					{
						_amountNotes += 50;
						_pNotes = realloc(_pNotes, _amountNotes);
					}
					_pNotes[_noteIndex] = atof(line);
					printf("note %i  %f\n", _noteIndex, _pNotes[_noteIndex]);
					break;
			}
		}
		_amountNotes = _noteIndex;
		_noteIndex = 0;
		fclose(_pFile);
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
	_musicTime += GetFrameTime();
	fixMusicTime();

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

		if(GetKeyPressed() && _musicTime!=0)
		{
			printf("keyPressed! \n");
			
			//todo fix
			_pNotes[_noteIndex] = _musicTime;
			printf("music Time: %.2f\n", _musicTime);
			//GetMusicTimePlayed(music);
			
			// printf("written value %f  to index: %i, supposed to be %f\n", notes[noteIndex], noteIndex,  GetMusicTimePlayed(music));
			_noteIndex++;
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			ClearBackground(BLACK);
		}
		// DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=noLessThanZero(fadeOut - GetMusicTimePlayed(music))*255});

	EndDrawing();
	if(endOfMusic())
	{
		saveFile(_noteIndex);
		_pGameplayFunction = &fMainMenu;
	}
}


void dNotes () 
{
	float width = GetScreenWidth() * 0.005;
	float middle = GetScreenWidth() /2;
	float scaleNotes = (float)(GetScreenWidth() / _noteTex.width) / 9;
	
	for(int i = _noteIndex; i >= 0 && _pNotes[i] + _scrollSpeed > _musicTime; i--)
	{
		if(i < 0) continue;
		//DrawCircle( middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
		//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
		float x = middle + middle * (_pNotes[i] - _musicTime) * (1/_scrollSpeed) - _noteTex.width * scaleNotes / 2;
		DrawTextureEx(_noteTex, (Vector2){.x=x, .y=GetScreenHeight() / 2 - _noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=128,.g=128,.b=128,.a= noLessThanZero(255-(255-(_pNotes[i] - _musicTime) * (255/_scrollSpeed)) / 2)});

	}

	if(_noteIndex < _amountNotes) //draw notes after line
	{
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

	fixMusicTime();

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
				if(scoreAdded > 200) {
					feedback("300!");
				}else if (scoreAdded > 100) {
					feedback("200!");
				} else {
					feedback("100!");
				}
				_score += scoreAdded;
			
				_noteIndex++;
				printf("new note index %i\n", _noteIndex);
				// PlaySoundMulti(hitSE);
				playAudioEffect(_pHitSE, _hitSE_Size);
			}else
			{
				printf("missed note\n");
				feedback("miss!");
				_fade = RED;
				_health -= _missPenalty;
				// PlaySoundMulti(missHitSE);
				playAudioEffect(_pMissHitSE, _missHitSE_Size);
			}
			ClearBackground(BLACK);
			//printf("health %f \n", _health);
		}

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
#define SUPPORT_FILEFORMAT_OGG

void fEditor ()
{
	int bpm = 150;
	static bool isPlaying = false;
	static float _scrollSpeed = 5;
	_musicPlaying = isPlaying;
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
		//printf("Mousewheel: %f  \n Key Right: %d", GetMouseWheelMove(), IsKeyDown(KEY_RIGHT));
	}
	//printf("Framecount: %d\n MusicTime: %f\n SongLength: %d\n", _musicFrameCount, _musicTime, _musicLength);
	if(_musicTime < 0)
		_musicTime = 0;

	if(_musicTime > getMusicDuration())
		_musicTime = getMusicDuration();
	
	
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
				printf("old\n");
				printAllNotes();
				printf("new\n");
				_amountNotes--;
				float * tmp = malloc(sizeof(float) * _amountNotes);
				for(int i = closestIndex; i < _amountNotes; i++)
				{
					tmp[i] = _pNotes[i+1];
				}
				free(_pNotes);
				_pNotes = tmp;
				printAllNotes();
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
		}


	EndDrawing();
	if(endOfMusic())
	{
		printAllNotes();
		char str [100];
		strcpy(str, _pMap);
		strcat(str, "/map.data");
		_pFile = fopen(str, "wb");
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
		drawButton(playButton,"play");

		Rectangle editorButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.5, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(editorButton,"Editor");

		Rectangle recordingButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.7, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(recordingButton,"Record");
		

		if(IsMouseButtonReleased(0) && mouseInRect(playButton))
		{
			//switching to playing map
			loadMap(0);
			printf("switching to playing map! \n");
			
			_pGameplayFunction = &fCountDown;
		}

		if(IsMouseButtonReleased(0) && mouseInRect(editorButton))
		{
			//switching to editing map
			loadMap(0);
			startMusic();
			_health = 50;
			_score = 0;
			_noteIndex =1;
			_musicTime = 0;
			printf("switching to editor map! \n");
			
			_pGameplayFunction = &fEditor;
		}

		if(IsMouseButtonReleased(0) && mouseInRect(recordingButton))
		{
			//switching to recording map
			loadMap(1);
			startMusic();
			_health = 50;
			_score = 0;
			_noteIndex =1;
			_musicTime = 0;
			printf("switching to recording map! \n");
			
			_pGameplayFunction = &fRecording;
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
	
	Vector2 mousePos;

	//todo do this smarter
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); //4 second long buffer
	ma_decoder tmp;
	_pHitSE = loadAudio("test.mp3", &tmp, &_hitSE_Size);
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
