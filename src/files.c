
#include "files.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../deps/raylib/src/raylib.h"
#include "windowsDefs.h"
#include "../deps/zip/src/zip.h"

#include "gameplay.h"
#include "audio.h"

#ifdef __unix
#include <sys/stat.h>
#define mkdir(dir) mkdir(dir, 0777)
#endif


extern Texture2D _background, _menuBackground, _noteTex;
extern Note * _pNotes;
extern float _scrollSpeed;
extern Map *_map;
extern int _amountNotes, _noteIndex, _score, _highestCombo;
extern bool _noBackground, _mapRefresh;
extern Settings _settings;
extern void ** _pMusic;
extern void *_pHitSE;
extern int _hitSE_Size;
extern float _averageAccuracy;
extern char _playerName[100];
//TODO add support for more maps
Map _pMaps [100];

FILE * _pFile;


Map loadMapInfo(char * file)
{
	char * mapStr = malloc(strlen(file) + 12);
	strcpy(mapStr, "maps/");
	strcat(mapStr, file);
	Map map = {0};
	map.zoom = 7;
	map.offset = 0;
	map.folder = malloc(100);
	map.musicLength = 0;
	map.musicFile = 0;
	map.music = 0;
	map.beats = 4;
	strcpy(map.folder, file);
	char * pStr = malloc(strlen(mapStr) + 30);
	map.imageFile = malloc(100);

	strcpy(pStr, mapStr);
	strcat(pStr, "/map.data");
	if(!FileExists(pStr))
		return (Map){.name=0};
	FILE * f;
	f = fopen(pStr, "r");

	map.name = calloc(sizeof(char), 100);
	map.name[0] = '\0';

	map.artist = calloc(sizeof(char), 100);
	map.artist[0] = '\0';

	map.mapCreator = calloc(sizeof(char), 100);
	map.mapCreator[0] = '\0';

	map.imageFile = calloc(sizeof(char), 100);
	map.imageFile[0] = '\0';

	map.musicFile = calloc(sizeof(char), 100);
	map.musicFile[0] = '\0';

	map.id = rand();



	
	//text file
	char line [1000] = {0};
	enum FilePart mode = fpNone;
	while(fgets(line,sizeof(line),f)!= 0)
	{
		int stringLenght = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLenght; i++)
			if(line[i] != ' ' && line[i] != '\n' && line[i] != '\r')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		for(int i = 0; i < 100; i++)
			if(line[i] == '\n' || line[i] == '\r' || !line[i])
				line[i]= '\0';
		
		if(strcmp(line, "[ID]") == 0)				{mode = fpID;			continue;}
		if(strcmp(line, "[Name]") == 0)				{mode = fpName;			continue;}
		if(strcmp(line, "[Artist]") == 0)			{mode = fpArtist;		continue;}
		if(strcmp(line, "[Map Creator]") == 0)		{mode = fpMapCreator;	continue;}
		if(strcmp(line, "[Difficulty]") == 0)		{mode = fpDifficulty;	continue;}
		if(strcmp(line, "[BPM]") == 0)				{mode = fpBPM;			continue;}
		if(strcmp(line, "[Image]") == 0)			{mode = fpImage;		continue;}
		if(strcmp(line, "[MusicFile]") == 0)		{mode = fpMusicFile;	continue;}
		if(strcmp(line, "[MusicLength]") == 0)		{mode = fpMusicLength;	continue;}
		if(strcmp(line, "[Zoom]") == 0)				{mode = fpZoom;			continue;}
		if(strcmp(line, "[Offset]") == 0)			{mode = fpOffset;		continue;}
		if(strcmp(line, "[Beats]") == 0)			{mode = fpBeats;		continue;}
		if(strcmp(line, "[Notes]") == 0)			{mode = fpNotes;		continue;}

		switch(mode)
		{
			case fpNone:
				break;
			case fpID:
				map.id = atoi(line);
			case fpName:
				strcpy(map.name, line);
				break;
			case fpArtist:
				strcpy(map.artist, line);
				break;
			case fpMapCreator:
				strcpy(map.mapCreator, line);
				break;
			case fpDifficulty:
				map.difficulty = atoi(line);
				break;
			case fpBPM:
				map.bpm = atoi(line);
				break;
			case fpImage:
				strcpy(map.imageFile, line);
				break;
			case fpMusicFile:
				strcpy(map.musicFile, line);
				break;
			case fpZoom:
				map.zoom = atoi(line);
				break;
			case fpMusicLength:
				map.musicLength = atoi(line);
				break;
			case fpOffset:
				map.offset = atoi(line);
				break;
			case fpBeats:
				map.beats = atof(line);
				break;
			case fpNotes:
				//neat, notes :P
				break;
		}
	}
	fclose(f);
	strcpy(pStr, mapStr);
	if(map.imageFile[0] != '\0')
		strcat(pStr, map.imageFile);
	else
		strcat(pStr, "/image.png");
	if(FileExists(pStr))
	{
		printf("image: %s \n", pStr);
		// map.image = LoadTexture(pStr);
		map.cpuImage = LoadImage(pStr);
		// strcpy(map.imageFile, pStr);
	}
	else{
		map.image = _menuBackground;
		map.cpuImage.width = 0;
	}
	SetTextureFilter(map.image, TEXTURE_FILTER_BILINEAR);
	free(pStr);
	return map;
}

void freeMap(Map * map)
{
	if(map->cpuImage.width!=0)
	{
		UnloadImage(map->cpuImage);
	}
	if(map->image.id != 0 && map->image.id != _menuBackground.id)
	{
		UnloadTexture(map->image);
	}
	if(map->name != 0)
		free(map->name);
	if(map->artist != 0)
		free(map->artist);
	if(map->mapCreator != 0)
		free(map->mapCreator);
	if(map->folder != 0)
		free(map->folder);
	if(map->imageFile != 0)
		free(map->imageFile);
	if(map->musicFile != 0)
		free(map->musicFile);
	map->bpm = 0;
	map->difficulty = 0;
	map->zoom = 7;
	map->offset = 0;
	map->musicLength = 0;
	map->id = 0;
}

void saveFile (int noteAmount)
{
	char str [100];
	strcpy(str, "maps/");
	strcat(str, _map->folder);
	strcat(str, "/map.data");
	printf("poggies: %s\n", str);
	_pFile = fopen(str, "w");
	printf("written map data\n");
	fprintf(_pFile, "[ID]\n");
	fprintf(_pFile, "%i\n", _map->id);
	fprintf(_pFile, "[Name]\n");
	fprintf(_pFile, "%s\n", _map->name);
	fprintf(_pFile, "[Artist]\n");
	fprintf(_pFile, "%s\n", _map->artist);
	fprintf(_pFile, "[Map Creator]\n");
	fprintf(_pFile, "%s\n", _map->mapCreator);
	fprintf(_pFile, "[Difficulty]\n");
	fprintf(_pFile, "%i\n", _map->difficulty);
	fprintf(_pFile, "[BPM]\n");
	fprintf(_pFile, "%i\n", _map->bpm);
	if(_map->imageFile != 0)
	{
		fprintf(_pFile, "[Image]\n");
		fprintf(_pFile, "%s\n", _map->imageFile);
	}
	fprintf(_pFile, "[MusicFile]\n");
	fprintf(_pFile, "%s\n", _map->musicFile);
	fprintf(_pFile, "[MusicLength]\n");
	fprintf(_pFile, "%i\n", _map->musicLength);
	fprintf(_pFile, "[Zoom]\n");
	fprintf(_pFile, "%i\n", _map->zoom);
	fprintf(_pFile, "[Offset]\n");
	fprintf(_pFile, "%i\n", _map->offset);
	fprintf(_pFile, "[Beats]\n");
	fprintf(_pFile, "%f\n", _map->beats);
	fprintf(_pFile, "[Notes]\n");
	
	for(int i = 0; i < noteAmount; i++)
	{
		if(_pNotes[i].time == 0)
			continue;
		
		fprintf(_pFile, "%f", _pNotes[i].time);
		if (_pNotes[i].hitSE_File != 0)
			fprintf(_pFile, " \"%s\"", _pNotes[i].hitSE_File);
		if (_pNotes[i].texture_File != 0)
			fprintf(_pFile, " \"%s\"",_pNotes[i].texture_File);
		if (_pNotes[i].anim && _pNotes[i].animSize)
		{
			fprintf(_pFile, " \"(%i",_pNotes[i].animSize);
			for(int j = 0; j < _pNotes[i].animSize; j++)
			{
				fprintf(_pFile, " %f,%f,%f", _pNotes[i].anim[j].time, _pNotes[i].anim[j].vec.x, _pNotes[i].anim[j].vec.y);
			}
			fprintf(_pFile, ")\"");
		}
		fprintf(_pFile, "\n");
	}
	fclose(_pFile);
	
}
//Load the map. Use 1 parameter to only open the file
void loadMap ()
{
	char * map = malloc(100);
	strcpy(map, "maps/");
	strcat(map, _map->folder);
	char * pStr = malloc(100);
	_background = _map->image;

	strcpy(pStr, map);
	if(_map->musicFile == 0)
	{
		_map->musicFile = malloc(100);
		strcpy(_map->musicFile, "/song.mp3");
	}
	if(_map->imageFile == 0)
	{
		_map->imageFile = malloc(100);
		strcpy(_map->imageFile, "/image.png");
	}
	strcat(pStr, _map->musicFile);

	_noBackground = 0;
	if(_map->image.id == _menuBackground.id)
		_noBackground = 1;

	// ma_result result
	if(_map->music==0)
		loadMusic(_map);
	_pMusic = &_map->music;
	_map->musicLength = (int)getMusicDuration();
	

	strcpy(pStr, map);
	strcat(pStr, "/map.data");
	_pFile = fopen(pStr, "r");

	//text file
	char line [1000];
	enum FilePart mode = fpNone;
	_amountNotes = 50;
	_noteIndex = 0;

	while(fgets(line,sizeof(line),_pFile)!= NULL)
	{
		int stringLenght = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLenght; i++)
			if(line[i] != ' ' && line[i] != '\n' && line[i] != '\r')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		if(strcmp(line, "[Notes]\n") == 0)			{mode = fpNotes;		continue;}
		switch(mode)
		{
			case fpNone:
				break;
			case fpNotes:
				// printf("new note %i\n", _noteIndex);
				if(_noteIndex <= _amountNotes)
				{
					_amountNotes += 50;
					_pNotes = realloc(_pNotes, _amountNotes*sizeof(Note));
				}
				_pNotes[_noteIndex].time = atof(line);
				_pNotes[_noteIndex].hitSE_File = 0;
				_pNotes[_noteIndex].texture_File = 0;
				_pNotes[_noteIndex].anim = 0;
				_pNotes[_noteIndex].animSize = 0;

				int part = 0;
				for(int j = 0; j < 2 && part != -1; j++)
				{
					for(int i = part+1; i < 1000; i++)
					{
						if(line[i] == '\r' || line[i] == '\n' || line[i] == '\0' || line[i] == 0)
						{
							part = -1;
							break;
						}
						if(line[i] == '"')
						{
							part = i+1;
							break;
						}
					}
					if(part == -1)
					{
						break;
					}

					//add texture file name
					char * tmpStr = malloc(100);
					int strPos = 0;
					for(int i = 0; i < 100; i++) //copy over file name
					{
						if(line[i+part] == '"' || line[i+part] == '\0')
						{
							tmpStr[strPos] = '\0';
							break;
						}
						tmpStr[strPos] = line[i+part];
						strPos++;
					}
					char * ext = GetFileExtension(tmpStr);
					if(ext != NULL)
					{
						if(strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0)
						{
							printf("found sound %s\n", tmpStr);
							//found hit sound
							if(_pNotes[_noteIndex].hitSE_File != 0)
								free(_pNotes[_noteIndex].hitSE_File);
							_pNotes[_noteIndex].hitSE_File = tmpStr;
						}else if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png")  == 0 || strcmp(ext, ".jpeg") == 0)
						{
							printf("found texture %s\n", tmpStr);
							//found texture
							if(_pNotes[_noteIndex].texture_File != 0)
								free(_pNotes[_noteIndex].texture_File);
							_pNotes[_noteIndex].texture_File = tmpStr;
						}
					}else if(line[part] == '(')
					{
						//found animation
						_pNotes[_noteIndex].animSize = atoi(&(line[part+1]));
						_pNotes[_noteIndex].anim = calloc(_pNotes[_noteIndex].animSize, sizeof(Frame));
						while(line[part] != ' ')
								part++;
						part++;
						printf("found animation %i  ", _pNotes[_noteIndex].animSize);
						for(int i = 0; i < _pNotes[_noteIndex].animSize; i++)
						{
							part++; //skip space
							_pNotes[_noteIndex].anim[i].time = atof(&(line[part]));
							while(line[part] != ',')
								part++;
							part++;
							_pNotes[_noteIndex].anim[i].vec.x = atof(&(line[part]));
							while(line[part] != ',')
								part++;
							part++;
							_pNotes[_noteIndex].anim[i].vec.y = atof(&(line[part]));
							while(line[part] != ' ')
								part++;
							printf("%f  %f  %f    ", _pNotes[_noteIndex].anim[i].time, _pNotes[_noteIndex].anim[i].vec.x, _pNotes[_noteIndex].anim[i].vec.y);
						}
						printf("\n");
					}
					for(int i = part; i < 1000; i++)
					{
						if(line[i] == '"')
						{
							part = i+1;
							break;
						}
					}
				}
				_noteIndex++;
				break;
		}
	}
	_amountNotes = _noteIndex;
	_noteIndex = 0;
	fclose(_pFile);
	free(pStr);

	//load all custom note textures & hit sounds
	//todo, dynamic allocation
	char textureFiles[5000][100];
	char soundFiles[5000][100];
	Texture textures[5000];
	static void *** sounds = 0;
	if(sounds == 0)
	{
		sounds = calloc(5000,sizeof(void*));
		printf("allocating sounds\n");
	}
	int soundLengths [5000];
	int amountTextures = 0, amountSounds = 0;
	for(int i = 0; i < _amountNotes; i++)
	{
		if(_pNotes[i].texture_File != 0)
		{
			int foundIndex = -1;
			for(int j = 0; j < amountTextures; j++)
			{
				if(strcmp(textureFiles[j], _pNotes[i].texture_File) == 0)
				{
					//found one
					foundIndex = j;
				}
			}
			if(foundIndex == -1)
			{
				//load texture
				strcpy(textureFiles[amountTextures], _pNotes[i].texture_File);
				char filePath [100];
				sprintf(filePath, "maps/%s%s", _map->folder, textureFiles[amountTextures]);
				textures[amountTextures] = LoadTexture(filePath);
				_pNotes[i].texture = textures[amountTextures];
				amountTextures++;
			}else
			{
				//already loaded
				_pNotes[i].texture = textures[amountTextures];
			}
		}else
			_pNotes[i].texture = _noteTex;

		if(_pNotes[i].hitSE_File != 0)
		{
			int foundIndex = -1;
			for(int j = 0; j < amountSounds; j++)
			{
				if(strcmp(soundFiles[j], _pNotes[i].hitSE_File) == 0)
				{
					//found one
					foundIndex = j;
				}
			}
			if(foundIndex == -1)
			{
				//load sound
				if(sounds[amountSounds] != 0)
					free(sounds[amountSounds]);
				strcpy(soundFiles[amountSounds], _pNotes[i].hitSE_File);
				char filePath [100];
				sprintf(filePath, "maps/%s%s", _map->folder, soundFiles[amountSounds]);
				if(sounds[amountSounds] == 0)
					sounds[amountSounds] = malloc(sizeof(void*));
				loadAudio(sounds[amountSounds], filePath, &(soundLengths[amountSounds]));
				_pNotes[i].hitSE = &sounds[amountSounds];
				_pNotes[i].hitSE_Length = &soundLengths[amountSounds];
				amountSounds++;
			}else
			{
				//already loaded
				_pNotes[i].hitSE = &sounds[amountSounds];
				_pNotes[i].hitSE_Length = &soundLengths[amountSounds];
			}
		}else
		{
			static void ** pHitSEp;
			pHitSEp = &_pHitSE;
			_pNotes[i].hitSE = &pHitSEp;
			_pNotes[i].hitSE_Length = &_hitSE_Size;
		}
	}
	// free(sounds);
	_noteIndex = 0;
	char str [100];
	sprintf(str, "%s - %s", _map->name, _map->artist);
	SetWindowTitle(str);
}

void saveScore()
{
	FILE * file;
	char str [100];
	if(!DirectoryExists("scores/"))
	{
		mkdir("scores");
	}
	sprintf(str, "scores/%s", _map->name);
	if(!DirectoryExists(str))
		mkdir(str);
	sprintf(str, "scores/%s/%s", _map->name, _playerName);
	printf("str %s\n", str);
	file = fopen(str, "w");
	fprintf(file, "%i %i %f", _score, _highestCombo, 100*(1-_averageAccuracy));
	fclose(file);
}

bool readScore(Map * map, int *score, int * combo, float * accuracy)
{
	*score = 0;
	*combo = 0;
	FILE * file;
	char str [100];
	sprintf(str, "scores/%s/%s", map->name, _playerName);
	if(!DirectoryExists("scores/"))
		return false;
	if(!FileExists(str))
		return false;
	file = fopen(str, "r");
	char line [1000];
	while(fgets(line,sizeof(line),file)!= NULL)
	{
		*score = atoi(line);
		char * part = &line[0];
		for(int i = 0; i < 1000; i++)
		{
			if(*part == 32)
			{
				part++;
				break;
			}
			part++;
		}
		*combo = atoi(part);
		for(int i = 0; i < 1000; i++)
		{
			if(*part == 32)
			{
				part++;
				break;
			}
			part++;
		}
		*accuracy = atof(part);
	}
	fclose(file);
	return true;
}

int on_extract_entry(const char *filename, void *arg) {
    static int i = 0;
    int n = *(int *)arg;
    printf("Extracted: %s (%d of %d)\n", filename, ++i, n);

    return 0;
}

void makeMapZip(Map * map)
{
	char str[200];
	strcpy(str, map->name);
	strcat(str, ".zip");
	struct zip_t *zip = zip_open(str, 6, 'w');
	strcpy(str, "maps/");
	strcat(str, map->folder);
	int amount = 0;
	char ** files = GetDirectoryFiles(str, &amount);
	for(int i = 0; i < amount; i++)
	{
		zip_entry_open(zip, files[i]);
		{
			strcpy(str, "maps/");
			strcat(str, map->folder);
			strcat(str, "/");
			strcat(str, files[i]);
			printf("compressing file %s\n", str);
			if(!FileExists(str))
				printf("wtf??\n");
			FILE * file = fopen(str, "r");
			fseek(file, 0L, SEEK_END);
			int size = ftell(file);
			rewind(file);
			char * buff = malloc(size);
			fread(buff, size, 1, file);
			fclose(file);
			zip_entry_write(zip, buff, size);
			free(buff);
		}
		zip_entry_close(zip);
	}
	zip_close(zip);
	ClearDirectoryFiles();
}

void addZipMap(char * file)
{
	int arg = 2;
	char str [100];
	strcpy(str, "maps/");
	strcat(str, GetFileNameWithoutExt(file));
	zip_extract(file, str, on_extract_entry, &arg);
	_mapRefresh = true;
}

void makeMap(Map * map)
{
	char * str = malloc(100);
	strcpy(str, "maps/");
	strcat(str, map->name);
	mkdir(str);
}

void unloadMap()
{
	// free(_pNotes);
	_amountNotes = 0;
	_noteIndex = 0;
}

void loadSettings()
{
	if(!FileExists("settings.conf"))
	{
		saveSettings();
		return;
	}
	FILE * f;
	f = fopen("settings.conf", "r");
	_settings.offset = 0;
	//text file
	char line [1000];
	enum SettingsPart mode = spNone;
	while(fgets(line,sizeof(line),f)!= 0)
	{
		int stringLenght = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLenght; i++)
			if(line[i] != ' ' && line[i] != '\n')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		if(strcmp(line, "[Name]\n") == 0)					{mode = spName;			continue;}
		if(strcmp(line, "[Volume Global]\n") == 0)			{mode = spVolGlobal;	continue;}
		if(strcmp(line, "[Volume Music]\n") == 0)			{mode = spVolMusic;		continue;}
		if(strcmp(line, "[Volume Sound Effects]\n") == 0)	{mode = spVolSE;		continue;}
		if(strcmp(line, "[Zoom]\n") == 0)					{mode = spZoom;			continue;}
		if(strcmp(line, "[Offset]\n") == 0)					{mode = spOffset;		continue;}
		for(int i = 0; i < 100; i++)
					if(line[i] == '\n') line[i]= '\0';
		switch(mode)
		{
			case spNone:
				break;
			case spName:
				strcpy(_playerName, line);
			case spVolGlobal:
				_settings.volumeGlobal = atoi(line);
				break;
			case spVolMusic:
				_settings.volumeMusic = atoi(line);
				break;
			case spVolSE:
				_settings.volumeSoundEffects = atoi(line);
				break;
			case spZoom:
				_settings.zoom = atoi(line);
				break;
			case spOffset:
				_settings.offset = atof(line);
				break;
		}
	}
	fclose(f);
	return;
}

void saveSettings ()
{
	printf("written settings\n");
	FILE * file = fopen("settings.conf", "w");
	fprintf(file, "[Name]\n");
	fprintf(file, "%s\n", _playerName);
	fprintf(file, "[Volume Global]\n");
	fprintf(file, "%i\n", _settings.volumeGlobal);
	fprintf(file, "[Volume Music]\n");
	fprintf(file, "%i\n", _settings.volumeMusic);
	fprintf(file, "[Volume Sound Effects]\n");
	fprintf(file, "%i\n", _settings.volumeSoundEffects);
	fprintf(file, "[Zoom]\n");
	fprintf(file, "%i\n", _settings.zoom);
	fprintf(file, "[Offset]\n");
	fprintf(file, "%f\n", _settings.offset);
	fclose(file);
}