#include <stdio.h>
//#include <stdlib.h>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio/miniaudio.h>

#include <raylib.h>


#define GLSL_VERSION            330


float * notes;
FILE * file;
void (*gameplayFunction)();
Texture2D background, heartTex, healthBarTex, noteTex, cursorTex;
// Music music;
// Sound hitSE, missHitSE, missSE;
int noBackground = 0;
float fadeOut= 0;
float health = 50;
int noteIndex = 1, amountNotes =0 ;
int score= 0;
char * map = "testMap";
float musicTime = 0;

float maxMargin = 0.1;
int hitPoints = 5;
int missPenalty = 10;
float scrollSpeed = 0.6;
Color fade = WHITE;


ma_decoder decoder;
ma_device_config deviceConfig;
ma_device device;

bool musicPlaying;


void fFail ();
void fCountDown ();
void fEndScreen ();
void fMainMenu();


int musicFrameCount = 0;
int musicLength = 0;
void* pMusic;
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount)
{
	if(!musicPlaying)
		return;
    ma_decoder* pDecoder = (ma_decoder*)pDevice->pUserData;
    if (pDecoder == NULL) {
        return;
    }
	if(musicLength > musicFrameCount)
		memcpy(pOutput, pMusic+musicFrameCount*sizeof(_Float32)*2, frameCount*sizeof(_Float32)*2);
	musicFrameCount += frameCount;


   
	printf("framecount %i  musicIndex %i musicLength %i\n", frameCount, musicFrameCount, musicLength);

    (void)pInput;
}

void resetBackGround()
{
	background = LoadTexture("background.png");
}


float noLessThanZero(float var)
{
	if(var < 0) return 0;
	return var;
}

void saveFile (int noteIndex)
{
	fwrite(notes, sizeof(float), noteIndex, file);
	printf("written map data\n");
}

void loadMusic(char * file)
{
	musicPlaying = false;
	if(musicLength != 0)
	{
		//unload previous music
		free(pMusic);
		ma_decoder_uninit(&decoder);
	}
	ma_result result;

	result = ma_decoder_init_file(file, NULL, &decoder);
    if (result != MA_SUCCESS) {
        printf("failed to open music file %s\n", file);
		exit(0);
    }
	printf("decoder format %i   Sizeof %i\n", decoder.outputFormat, sizeof(_Float32));
	int lastFrame = -1;
	int musicSize = 10;
	musicLength = ma_decoder_get_length_in_pcm_frames(&decoder);
	printf("Music length %i\n", musicLength);
	pMusic = calloc(sizeof(_Float32)*2, musicLength);
	void * pCursor = pMusic;
	while(decoder.readPointerInPCMFrames !=lastFrame)
	{
		lastFrame = decoder.readPointerInPCMFrames;
		ma_decoder_read_pcm_frames(&decoder, pCursor, 256);
		int size = 0;
		while(*(int*)(pCursor + size) != 0)
		{
			size++;
		}
		// printf("size %i \t pCursor %i\n", size, *(int*)(pCursor + size));
		pCursor += sizeof(_Float32)*2*256;
	}


	deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format   = decoder.outputFormat;
    deviceConfig.playback.channels = decoder.outputChannels;
    deviceConfig.sampleRate        = decoder.outputSampleRate;
    deviceConfig.dataCallback      = data_callback;
    deviceConfig.pUserData         = &decoder;
	
}

void startMusic()
{
	if(musicFrameCount != 0)
	{
		//unload previous devices
		ma_device_uninit(&device);
   		ma_decoder_uninit(&decoder);
	}
	if (ma_device_init(NULL, &deviceConfig, &device) != MA_SUCCESS) {
			printf("Failed to open playback device.\n");
			ma_decoder_uninit(&decoder);
			return;
	}

	if (ma_device_start(&device) != MA_SUCCESS) {
		printf("Failed to start playback device.\n");
		ma_device_uninit(&device);
		ma_decoder_uninit(&decoder);
		return;
	}
	musicPlaying = true;
}

void loadMap (int fileType)
{
	char * tmp = malloc(strlen(map) + 12);
	strcpy(tmp, map);
	strcat(tmp, "/image.png");
	background = LoadTexture(tmp);
	strcpy(tmp, map);
	strcat(tmp, "/music.mp3");

	// ma_result result
	// music = LoadMusicStream(tmp);
	loadMusic(tmp);
	

	strcpy(tmp, map);
	strcat(tmp, "/map.data");
	if(fileType == 0)
	{
		file = fopen(tmp, "rb");
		float size;
		fread(&size, 1, sizeof(float), file);
		printf("size: %i", (int)size);
		if(amountNotes != 0)
		{
			free(notes);
		}
		notes = calloc(size, sizeof(float));
		rewind (file);
		fread(notes, size, sizeof(float), file);
		fclose(file);
		amountNotes = size;
	}else
	{
		file = fopen(tmp, "wb");
		notes = calloc(100000, sizeof(float));
	}
	free(tmp);

	if(background.id == 0)
	{
		printf("no background texture found\n");
		background = LoadTexture("background.png");
		noBackground = 1;
	}

	
	
}

void drawCursor ()
{
	static float lastClick;
	if(IsMouseButtonDown(0))
	{
		lastClick = GetTime();
	}
	float size = (GetScreenWidth() * (0.06 - 0.1 * noLessThanZero(0.06 + (lastClick - GetTime())) )) / cursorTex.width;
	DrawTextureEx(cursorTex, (Vector2){.x=GetMouseX(), .y=GetMouseY()}, 0, size, WHITE);
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
		if(!noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)background.width;
			DrawTextureEx(background, (Vector2){.x=0, .y=(GetScreenHeight() - background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}

		if(GetKeyPressed())
		{
			printf("keyPressed! \n");
			
			//todo fix
			notes[noteIndex] = 0;
			//GetMusicTimePlayed(music);
			
			// printf("written value %f  to index: %i, supposed to be %f\n", notes[noteIndex], noteIndex,  GetMusicTimePlayed(music));
			noteIndex++;
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
	float scaleNotes = (float)(GetScreenWidth() / noteTex.width) / 9;
	if(noteIndex < amountNotes)//draw notes after line
	{
		for(int i = noteIndex; i >= 0 && notes[i] + scrollSpeed > musicTime; i--)
		{
			if(i < 0) continue;
			//DrawCircle( middle + middle * (notes[i] - musicTime) * (1/scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (notes[i] - musicTime) * (1/scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			float x = middle + middle * (notes[i] - musicTime) * (1/scrollSpeed) - noteTex.width * scaleNotes / 2;
			DrawTextureEx(noteTex, (Vector2){.x=x, .y=GetScreenHeight() / 2 - noteTex.height * scaleNotes}, 0,  scaleNotes,(Color){.r=128,.g=128,.b=128,.a= noLessThanZero(255-(notes[i] - musicTime) * (255/scrollSpeed)) / 2});

		}

		//draw notes before line
		for(int i = noteIndex; i < amountNotes && notes[i] - scrollSpeed < musicTime; i++)
		{
			//DrawCircle( middle + middle * (notes[i] - musicTime) * (1/scrollSpeed) ,GetScreenHeight() / 2, GetScreenWidth() / 20, WHITE);
			//DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (notes[i] - musicTime) * (1/scrollSpeed), .y=GetScreenHeight() / 2}, 0, GetScreenWidth() / 20,WHITE);
			DrawTextureEx(noteTex, (Vector2){.x=middle + middle * (notes[i] - musicTime) * (1/scrollSpeed) - noteTex.width * scaleNotes / 2, .y=GetScreenHeight() / 2 - noteTex.height * scaleNotes}, 0,  scaleNotes,WHITE);
		}
	}
	DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=255/2});
}

#define feedback(newFeedback) feedbackSayings[feedbackIndex] = newFeedback; feedbackIndex++; if(feedbackIndex > 4) feedbackIndex = 0;
void fPlaying ()
{
	static char *feedbackSayings [5];
	static int feedbackIndex = 0;
	// UpdateMusicStream(music);
	musicTime += GetFrameTime();
	// if(GetMusicTimePlayed(music) > musicTime)
	// 	musicTime = GetMusicTimePlayed(music);
	// if(GetMusicTimePlayed(music) < musicTime - 0.1)
	// 	musicTime = GetMusicTimePlayed(music);
	// if(GetMusicTimePlayed(music)/GetMusicTimeLength(music) > 0.99)
	// {
	// 	//goto endScreen
	// 	gameplayFunction = &fEndScreen;
	// 	StopMusicStream(music);
	// 	return;
	// }
	BeginDrawing();
		ClearBackground(BLACK);
		
		if(!noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)background.width;
			DrawTextureEx(background, (Vector2){.x=0, .y=(GetScreenHeight() - background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}
		//DrawFPS(500,0);

		//draw notes
		float width = GetScreenWidth() * 0.005;
		float middle = GetScreenWidth() /2;
		
		float scaleNotes = (float)(GetScreenWidth() / noteTex.width) / 9;

		//draw score
		char * tmpString = malloc(9);
		sprintf(tmpString, "%i", score);
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
		if(noteIndex < amountNotes && musicTime - maxMargin > notes[noteIndex])
		{
			//passed note
			noteIndex++;
			health -= missPenalty;
			printf("missed note %f  index %i   note: %f\n", musicTime, noteIndex, notes[noteIndex]);
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			fade = RED;
			feedback("miss!");
		}

		if(GetKeyPressed() && noteIndex < amountNotes)
		{
			//printf("keyPressed! \n");
			
			// fadeOut = GetMusicTimePlayed(music) + 0.1;
			fade = WHITE;
			float closestTime = 55;
			int closestIndex = 0;
			for(int i = noteIndex; i <= noteIndex + 1 && i < amountNotes; i++)
			{
				//printf("fabs: %f\n",fabs(notes[i] - musicTime));
				if(closestTime > fabs(notes[i] - musicTime))
				{
					closestTime = fabs(notes[i] - musicTime);
					closestIndex = i;
				}
			}
			//printf("closestTime: %f\n", closestTime);
			if(closestTime < maxMargin)
			{
				printf("hit note! %i\n", noteIndex);
				while(noteIndex < closestIndex)
				{
					noteIndex++;
					health -= missPenalty;
					feedback("miss!");
				}
				int healthAdded = noLessThanZero(hitPoints - closestTime * (hitPoints / maxMargin));
				health += healthAdded;
				//feedback("hit!");
				int scoreAdded = noLessThanZero(300 - closestTime * (300 / maxMargin));
				if(score > 200) {
					feedback("300!");
				}else if (score > 100) {
					feedback("200!");
				} else {
					feedback("100!");
				}
				score += scoreAdded;
			
				noteIndex++;
				printf("new note index %i\n", noteIndex);
				// PlaySoundMulti(hitSE);
			}else
			{
				printf("missed note\n");
				feedback("miss!");
				fade = RED;
				health -= missPenalty;
				// PlaySoundMulti(missHitSE);
			}
			ClearBackground(BLACK);
			printf("health %f \n", health);
		}
		if(health > 100)
			health = 100;
		if(health < 0)
			health = 0;

		if(feedbackIndex >= 5)
			feedbackIndex = 0;

		float heartScale = (GetScreenWidth() * 0.08) / heartTex.width;
		float healthBarScale = (GetScreenHeight() * 0.4) / healthBarTex.height;
		//DrawTextureEx(heartTex, (Vector2){.x=0, .y=0}, 0, heartScale,WHITE);
		DrawTextureEx(healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (heartTex.width / 2) * heartScale - (healthBarTex.width / 2) * healthBarScale,
			.y=GetScreenHeight() * 0.85 - healthBarTex.height * healthBarScale + (heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
		DrawTextureEx(heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - health / 250)}, 0, heartScale,WHITE);
		
		
		// DrawRectangle(0,0, GetScreenWidth(), GetScreenHeight(), (Color){.r=fade.r,.g=fade.g,.b=fade.b,.a=noLessThanZero(fadeOut - GetMusicTimePlayed(music))*255});

		if(health <= 0)
		{
			//goto fFail
			gameplayFunction = &fFail;
			// StopMusicStream(music);
		}

	EndDrawing();
}

void fFail ()
{
	BeginDrawing();
		ClearBackground(BLACK);
		if(!noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)background.width;
			DrawTextureEx(background, (Vector2){.x=0, .y=(GetScreenHeight() - background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
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
		sprintf(tmpString, "%i", score);
		textSize = MeasureText(tmpString, GetScreenWidth() * 0.1);
		DrawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.5, GetScreenWidth() * 0.1, LIGHTGRAY);
		free(tmpString);

		if(IsMouseButtonReleased(0) && mouseInRect(playButton))
		{
			//retrying map
			printf("retrying map! \n");
			gameplayFunction = &fCountDown;
		}
		if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
		{
			//retrying map
			printf("going to main Menu! \n");
			gameplayFunction = &fMainMenu;
			resetBackGround();
		}
		drawCursor();

	EndDrawing();
}

void fEndScreen ()
{
	BeginDrawing();
		ClearBackground(BLACK);
		if(!noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)background.width;
			DrawTextureEx(background, (Vector2){.x=0, .y=(GetScreenHeight() - background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
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
		sprintf(tmpString, "%i", score);
		textSize = MeasureText(tmpString, GetScreenWidth() * 0.1);
		DrawText(tmpString, GetScreenWidth() * 0.5 - textSize / 2, GetScreenHeight()*0.5, GetScreenWidth() * 0.1, LIGHTGRAY);
		free(tmpString);

		if(IsMouseButtonReleased(0) && mouseInRect(playButton))
		{
			//retrying map
			printf("retrying map! \n");
			gameplayFunction = &fCountDown;
		}
		if(IsMouseButtonReleased(0) && mouseInRect(MMButton))
		{
			//retrying map
			printf("going to main Menu! \n");
			gameplayFunction = &fMainMenu;
			resetBackGround();
		}
		drawCursor();

	EndDrawing();
}


void fCountDown ()
{
	static float countDown  = 0;
	if(countDown == 0) countDown = GetTime() + 3;
	// printf("countDown %f  getTime %f\n", countDown, GetTime());
	if(countDown - GetTime() < 0)
	{
		countDown = 0;
		//switching to playing map
		printf("switching to playing map! \n");
		gameplayFunction = &fPlaying;

		startMusic();
		
		
		// PlayMusicStream(music);  
		// SetMusicVolume(music, 0.8);
		health = 50;
		score = 0;
		noteIndex =1;
		musicTime = 0;
	}
	BeginDrawing();
		ClearBackground(BLACK);
		if(!noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)background.width;
			DrawTextureEx(background, (Vector2){.x=0, .y=(GetScreenHeight() - background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}
		//DrawFPS(500,0);

		//drawing parts of UI to not be too jarring when countdown is over

		//draw notes
		float width = GetScreenWidth() * 0.005;
		float middle = GetScreenWidth() /2;
		
		
		float scaleNotes = (float)(GetScreenWidth() / noteTex.width) / 9;

		DrawRectangle(middle - width / 2,0 , width, GetScreenHeight(), (Color){.r=255,.g=255,.b=255,.a=255/2});

		//draw score
		char * tmpString = malloc(9);
		sprintf(tmpString, "%i", score);
		DrawText(tmpString, GetScreenWidth() * 0.05, GetScreenHeight()*0.05, GetScreenWidth() * 0.05, WHITE);
		
		drawCursor();
		
		float heartScale = (GetScreenWidth() * 0.08) / heartTex.width;
		float healthBarScale = (GetScreenHeight() * 0.4) / healthBarTex.height;
		//DrawTextureEx(heartTex, (Vector2){.x=0, .y=0}, 0, heartScale,WHITE);
		DrawTextureEx(healthBarTex, (Vector2){.x=GetScreenWidth() * 0.85 + (heartTex.width / 2) * heartScale - (healthBarTex.width / 2) * healthBarScale,
			.y=GetScreenHeight() * 0.85 - healthBarTex.height * healthBarScale + (heartTex.height / 2) * heartScale}, 0, healthBarScale,WHITE);
		DrawTextureEx(heartTex, (Vector2){.x=GetScreenWidth() * 0.85, .y=GetScreenHeight() * (0.85 - health / 250)}, 0, heartScale,WHITE);
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
	static float scrollSpeed = 1;
	musicPlaying = isPlaying;
	printf("musicTime: %.2f \t rMusicTime: %.2f\n", musicTime, (float)musicFrameCount/(decoder.outputSampleRate));
	if(isPlaying) {
		// UpdateMusicStreamCustom(music);
		musicTime += GetFrameTime();
		// if(GetMusicTimePlayed(music) > musicTime)
		// 	musicTime = GetMusicTimePlayed(music);
		// if(GetMusicTimePlayed(music) < musicTime - 0.1)
		// 	musicTime = GetMusicTimePlayed(music);
		// if(GetMusicTimePlayed(music)/GetMusicTimeLength(music) > 0.99)
		// {
		// 	isPlaying = false;
		// }
	}else
	{
		musicFrameCount = musicTime*decoder.outputSampleRate;
	}
	if(IsKeyDown(KEY_RIGHT)) musicTime+= GetFrameTime()*scrollSpeed;
	if(IsKeyDown(KEY_LEFT)) musicTime-= GetFrameTime()*scrollSpeed;
	if(IsKeyPressed(KEY_UP)) scrollSpeed *= 1.2;
	if(IsKeyPressed(KEY_DOWN)) scrollSpeed /= 1.2;
	if(scrollSpeed == 0) scrollSpeed = 0.01;

	BeginDrawing();
		ClearBackground(BLACK);
		
		if(!noBackground)
		{
			float scale = (float)GetScreenWidth() / (float)background.width;
			DrawTextureEx(background, (Vector2){.x=0, .y=(GetScreenHeight() - background.height * scale)/2}, 0, scale,WHITE);
		}else{
			DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
			(Rectangle){.x=0, .y=0, .height = GetScreenHeight(), .width= GetScreenWidth()}, (Vector2){.x=0, .y=0}, 0, 0.2, WHITE);
		}
		//DrawFPS(500,0);

		//draw notes
		float width = GetScreenWidth() * 0.005;
		float middle = GetScreenWidth() /2;
		
		float scaleNotes = (float)(GetScreenWidth() / noteTex.width) / 9;

		dNotes();

		float closestTime = 55;
		int closestIndex = 0;
		for(int i = 0; i < amountNotes; i++)
		{
			if(closestTime > fabs(notes[i] - musicTime))
			{
				closestTime = fabs(notes[i] - musicTime);
				closestIndex = i;
			}
		}
		if(closestTime < maxMargin && IsKeyPressed(KEY_X))
		{
			amountNotes--;
			for(int i = closestIndex; i < amountNotes; i++)
			{
				notes[i] = notes[i+1];
			}
			float * tmp = malloc(sizeof(float) * amountNotes);
			for(int i = 0; i < amountNotes; i++)
			{
				tmp[i] = notes[i];
			}
			free(notes);
			notes = tmp;
		}

		if(IsKeyPressed(KEY_Z))
		{
			amountNotes++;
			float * tmp = calloc(amountNotes, sizeof(float));
			for(int i = 0; i < amountNotes-1; i++)
			{
				tmp[i] = notes[i];
			}
			for(int i = closestIndex; i < amountNotes - 1; i++)
			{
				tmp[i+1] = notes[i];
			}
			
			
			free(notes);
			notes = tmp;
			notes[closestIndex] = musicTime;
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


		// if(GetKeyPressed() && noteIndex < amountNotes)
		// {
		// 	//printf("keyPressed! \n");
			
		// 	fadeOut = GetMusicTimePlayed(music) + 0.1;
		// 	fade = WHITE;
		// 	float closestTime = 55;
		// 	int closestIndex = 0;
		// 	for(int i = noteIndex; i <= noteIndex + 1 && i < amountNotes; i++)
		// 	{
		// 		//printf("fabs: %f\n",fabs(notes[i] - musicTime));
		// 		if(closestTime > fabs(notes[i] - musicTime))
		// 		{
		// 			closestTime = fabs(notes[i] - musicTime);
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
	BeginDrawing();
		ClearBackground(BLACK);
		DrawTextureTiled(background, (Rectangle){.x=GetTime()*50, .y=GetTime()*50, .height = background.height, .width= background.width},
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
			gameplayFunction = &fCountDown;
		}

		if(IsMouseButtonReleased(0) && mouseInRect(editorButton))
		{
			//switching to playing map
			loadMap(0);
			startMusic();
			// PlayMusicStream(music);  
			// SetMusicVolume(music, 0.8);
			health = 50;
			score = 0;
			noteIndex =1;
			musicTime = 0;
			printf("switching to editor map! \n");
			gameplayFunction = &fEditor;
		}

		drawCursor();

	EndDrawing();
}

int main (int argc, char **argv)
{
	
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
	
	heartTex = LoadTexture("heart.png");
	healthBarTex = LoadTexture("healthBar.png");
	noteTex = LoadTexture("note.png");
	cursorTex = LoadTexture("cursor.png");
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
	// 	gameplayFunction = &fRecording;
	// }else
	// {
	// 	gameplayFunction = &fPlaying;
	// }
	
	gameplayFunction = &fMainMenu;
	
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

		(*gameplayFunction)();
		
		

	}
	// if(0 && GetMusicTimePlayed(music)/GetMusicTimeLength(music) >= 0.99)
	// {
	// 	notes[0] = noteIndex;
	// 	saveFile(noteIndex);
	// }

	UnloadTexture(background);
	CloseWindow();
	return 0;

}
