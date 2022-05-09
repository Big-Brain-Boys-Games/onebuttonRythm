
#include "files.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "deps/raylib/src/raylib.h"
#include "deps/zip/src/zip.h"
#include "gameplay.h"



#ifdef _WIN32
#include <windows.h>

#define mkdir(dir) _mkdir(dir)
#endif
#ifdef __unix
#include <sys/stat.h>
#define mkdir(dir) mkdir(dir, 0777)
#endif


extern Texture2D _background, _menuBackground;
extern float * _pNotes, _scrollSpeed;
extern Map *_map;
extern int _amountNotes, _noteIndex, _score, _highestCombo;
extern bool _noBackground, _mapRefresh;
extern Settings _settings;
extern void ** _pMusic;
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
	strcpy(map.folder, file);
	char * pStr = malloc(strlen(mapStr) + 30);

	strcpy(pStr, mapStr);
	strcat(pStr, "/map.data");
	if(!FileExists(pStr))
		return (Map){.name=0};
	FILE * f;
	f = fopen(pStr, "rb");

	
	//text file
	char line [1000];
	enum FilePart mode = fpNone;
	while(fgets(line,sizeof(line),f)!= 0)
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
		if(strcmp(line, "[Image]\n") == 0)			{mode = fpImage;		continue;}
		if(strcmp(line, "[MusicFile]\n") == 0)		{mode = fpMusicFile;	continue;}
		if(strcmp(line, "[MusicLength]\n") == 0)	{mode = fpMusicLength;	continue;}
		if(strcmp(line, "[Zoom]\n") == 0)			{mode = fpZoom;			continue;}
		if(strcmp(line, "[Offset]\n") == 0)			{mode = fpOffset;		continue;}
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
				break;
			case fpDifficulty:
				map.difficulty = atoi(line);
				break;
			case fpBPM:
				map.bpm = atoi(line);
				break;
			case fpImage:
				map.imageFile = malloc(100);
				strcpy(map.imageFile, line);
				break;
			case fpMusicFile:
				map.musicFile = malloc(100);
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
			case fpNotes:
				//neat, notes :P
				break;
		}
	}
	fclose(f);
	strcpy(pStr, mapStr);
	if(map.imageFile != 0)
		strcat(pStr, map.imageFile);
	else
		strcat(pStr, "/image.png");
	if(FileExists(pStr))
	{
		printf("image: %s \n", pStr);
		// map.image = LoadTexture(pStr);
		map.cpuImage = LoadImage(pStr);
		map.imageFile = malloc(100);
		strcpy(map.imageFile, pStr);
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
	if(map->creator != 0)
		free(map->creator);
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
}

void saveFile (int noteAmount)
{
	char str [100];
	strcpy(str, "maps/");
	strcat(str, _map->folder);
	strcat(str, "/map.data");
	_pFile = fopen(str, "");
	printf("written map data\n");
	fprintf(_pFile, "[Name]\n");
	fprintf(_pFile, "%s\n", _map->name);
	fprintf(_pFile, "[Creator]\n");
	fprintf(_pFile, "%s\n", _map->creator);
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
	fprintf(_pFile, "[Notes]\n");
	for(int i = 0; i < noteAmount; i++)
	{
		if(_pNotes[i] == 0)
			continue;
		fprintf(_pFile, "%f\n", _pNotes[i]);
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
	_pFile = fopen(pStr, "rb");

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
			if(line[i] != ' ' && line[i] != '\n')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		if(strcmp(line, "[Notes]\n") == 0)			{mode = fpNotes;		continue;}
		switch(mode)
		{
			case fpNone:
				break;
			case fpNotes:
				
				if(_noteIndex <= _amountNotes)
				{
					_amountNotes += 50;
					_pNotes = realloc(_pNotes, _amountNotes);
				}
				_pNotes[_noteIndex] = atof(line);
				_noteIndex++;
				break;
		}
	}
	_amountNotes = _noteIndex;
	_noteIndex = 0;
	fclose(_pFile);
	free(pStr);
}

void saveScore()
{
	FILE * file;
	char str [100];
	strcpy(str, "scores/");
	strcat(str, _map->name);
	if(!DirectoryExists("scores/"))
		return;
	printf("str %s\n", str);
	file = fopen(str, "w");
	//todo add combo
	fprintf(file, "%i %i\n", _score, _highestCombo);
	fclose(file);
}

bool readScore(Map * map, int *score, int * combo)
{
	*score = 0;
	*combo = 0;
	FILE * file;
	char str [100];
	strcpy(str, "scores/");
	strcat(str, map->name);
	if(!DirectoryExists("scores/"))
		return false;
	if(!FileExists(str))
		return false;
	file = fopen(str, "r");
	//todo add combo
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
	struct zip_t *zip = zip_open(str, ZIP_DEFAULT_COMPRESSION_LEVEL, 'w');
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
		saveSettings();
		return;
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