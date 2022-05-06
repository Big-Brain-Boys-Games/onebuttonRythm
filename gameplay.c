#ifndef MAINC
#include "gameplay.h"
#endif

bool endOfMusic()
{
	if (_musicLength < _musicFrameCount)
		return true;
	return false;
}

void playAudioEffect(void *effect, int size)
{
	for (int i = 0; i < size; i++)
	{
		((_Float32 *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] += ((_Float32 *)effect)[i];
	}
}

void startMusic()
{
	if (_musicFrameCount != 0)
	{
		// unload previous devices
		ma_device_uninit(&_device);
	}
	if (ma_device_init(NULL, &_deviceConfig, &_device) != MA_SUCCESS)
	{
		printf("Failed to open playback device.\n");
		return;
	}

	if (ma_device_start(&_device) != MA_SUCCESS)
	{
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
	free(_pMusic);
}

void unloadMap()
{
	free(_pNotes);
	_amountNotes = 0;
	_noteIndex = 0;
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
	BeginDrawing();
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
				_pGameplayFunction = _pNextGameplayFunction;
			}
			if(mouseInRect(exitButton))
			{
				stopMusic();
				unloadMap();
				_pGameplayFunction = &fMainMenu;
				resetBackGround();
			}
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
		_musicHead = 0;
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
		drawVignette();
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
		drawButton(playButton,"play", 0.05);

		Rectangle editorButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.5, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(editorButton,"Editor", 0.05);

		Rectangle recordingButton = (Rectangle){.x=middle - GetScreenWidth()*0.15, .y=GetScreenHeight() * 0.7, .width=GetScreenWidth()*0.3,.height=GetScreenHeight()*0.1};
		drawButton(recordingButton,"Record", 0.05);
		

		if(IsMouseButtonReleased(0) && mouseInRect(playButton))
		{
			//switching to playing map
			printf("switching to playing map! \n");
			
			_pNextGameplayFunction = &fCountDown;
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
			printf("switching to editor map! \n");
			
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
			unloadMap();
			stopMusic();
			_pGameplayFunction = &fMainMenu;
			resetBackGround();
		}
		drawVignette();
		drawCursor();

	EndDrawing();
}

void fEditor ()
{
	static bool isPlaying = false;
	_musicPlaying = isPlaying;
	if(isPlaying) {
		// UpdateMusicStreamCustom(music);
		_musicHead += GetFrameTime();
	}else
	{
		_musicFrameCount = _musicHead*_decoder.outputSampleRate;
		if(IsKeyDown(KEY_RIGHT) || GetMouseWheelMove() > 0) _musicHead+= GetFrameTime()*_scrollSpeed;
		if(IsKeyDown(KEY_LEFT) || GetMouseWheelMove() < 0) _musicHead-= GetFrameTime()*_scrollSpeed;
		if(IsKeyPressed(KEY_UP)) _scrollSpeed *= 1.2;
		if(IsKeyPressed(KEY_DOWN)) _scrollSpeed /= 1.2;
		if(_scrollSpeed == 0) _scrollSpeed = 0.01;
		//printf("Mousewheel: %f  \n Key Right: %d", GetMouseWheelMove(), IsKeyDown(KEY_RIGHT));
	}
	//printf("Framecount: %d\n MusicTime: %f\n SongLength: %d\n", _musicFrameCount, _musicHead, _musicLength);
	if(_musicHead < 0)
		_musicHead = 0;

	if(IsKeyPressed(KEY_ESCAPE)) {
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fPlaying;
	}

	if(_musicHead > getMusicDuration())
		_musicHead = getMusicDuration();
	

	// printf("Bars: %i\t music length: %i\t current time: %f\n", getBarsCount(), _musicLength, _musicTime);

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
			printf("poggers making new note\n");
			_amountNotes++;
			float * tmp = calloc(_amountNotes, sizeof(float));
			for(int i = 0; i < _amountNotes-1; i++)
			{
				tmp[i] = _pNotes[i];
				if(tmp[i] < _musicHead)
					closestIndex=i;
			}
			for(int i = closestIndex+1; i < _amountNotes-1; i++)
			{
				tmp[i+1] = _pNotes[i];
			}
			
			
			free(_pNotes);
			_pNotes = tmp;
			_pNotes[closestIndex] = _musicHead;
			printf("amount %i, new note %.2f index: %i\n", _amountNotes, _pNotes[closestIndex], closestIndex);
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

		drawMusicGraph(0.7);
		drawVignette();
		drawBars();
		drawProgressBarI(true);

	EndDrawing();
	if(endOfMusic())
	{
		loadMap(1);
		stopMusic();
		_pGameplayFunction = &fEndScreen;
		resetBackGround();
	}
}

void fRecording ()
{
	_musicHead += GetFrameTime();
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

		if(GetKeyPressed() && _musicHead!=0)
		{
			printf("keyPressed! \n");
			
			newNote(_musicHead);
			printf("music Time: %.2f\n", _musicHead);
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			ClearBackground(BLACK);
		}
		// DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=noLessThanZero(fadeOut - GetMusicTimePlayed(music))*255});
		dNotes();
		drawVignette();
		drawBars();
		drawProgressBar();
	EndDrawing();
	if(endOfMusic())
	{
		saveFile(_amountNotes);
		unloadMap();
		stopMusic();
		_pGameplayFunction = &fMainMenu;
		resetBackGround();
	}
}

#define feedback(newFeedback) feedbackSayings[feedbackIndex] = newFeedback; feedbackIndex++; if(feedbackIndex > 4) feedbackIndex = 0;
void fPlaying ()
{
	static char *feedbackSayings [5];
	static int feedbackIndex = 0;
	_musicHead += GetFrameTime();

	_musicPlaying = true;
	fixMusicTime();

	// _pGameplayFunction = &fEndScreen;

	if(IsKeyPressed(KEY_ESCAPE))
	{
		_pGameplayFunction = &fPause;
		_pNextGameplayFunction = &fPlaying;
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
		if(_noteIndex < _amountNotes && _musicHead - _maxMargin > _pNotes[_noteIndex])
		{
			//passed note
			_noteIndex++;
			_health -= _missPenalty;
			printf("missed note %f  index %i   note: %f\n", _musicHead, _noteIndex, _pNotes[_noteIndex]);
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
				if(closestTime > fabs(_pNotes[i] - _musicHead))
				{
					closestTime = fabs(_pNotes[i] - _musicHead);
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
			stopMusic();
			// StopMusicStream(music);
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
			//retrying map
			printf("retrying map! \n");
			_pGameplayFunction = &fCountDown;
		}
		if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
		{
			//retrying map
			printf("going to main Menu! \n");
			unloadMap();
			stopMusic();
			_pGameplayFunction = &fMainMenu;
			resetBackGround();
		}
		drawVignette();
		drawCursor();

	EndDrawing();
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
		printf("\nmaps: \n");
		for(int i = 0; i < amount; i++)
		{
			if(files[i][0] == '.')
				continue;
			printf("file %i - %s\n", i, files[i]);
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
			drawMapThumbnail(mapButton,&_pMaps[i]);

			if(IsMouseButtonReleased(0) && mouseInRect(mapButton))
			{
				_pMap = malloc(100);
				strcpy(_pMap, _pMaps[i].folder);
				//switching to playing map
				if(_pNextGameplayFunction != &fRecording)
					loadMap(0);
				else
					loadMap(1);
				
				if(_pNextGameplayFunction != &fCountDown)
					startMusic();
				printf("selected map!\n");
				
				_pGameplayFunction = _pNextGameplayFunction;
			}
		}
		drawCursor();
	EndDrawing();
}