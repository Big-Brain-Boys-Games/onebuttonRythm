
#include "files.h"

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "include/raylib.h"


#ifdef _WIN32
#include <windows.h>

#define mkdir(dir) _mkdir(dir)
#endif
#ifdef __unix
#include <sys/stat.h>
#define mkdir(dir) mkdir(dir, 0777)
#endif


extern Texture2D _background;
extern float * _pNotes;
extern Map *_map;
extern int _amountNotes, _noteIndex, _score, _highestCombo;
extern bool _noBackground;
//TODO add support for more maps
Map _pMaps [100];

FILE * _pFile;


Map loadMapInfo(char * file)
{
	char * mapStr = malloc(strlen(file) + 12);
	strcpy(mapStr, "maps/");
	strcat(mapStr, file);
	Map map = {0};
	map.folder = malloc(100);
	strcpy(map.folder, file);
	char * pStr = malloc(strlen(mapStr) + 12);

	strcpy(pStr, mapStr);
	strcat(pStr, "/map.data");
	if(!FileExists(pStr))
		return (Map){.name=0};
	FILE * f;
	f = fopen(pStr, "rb");

	strcpy(pStr, mapStr);
	strcat(pStr, "/image.png");
	if(FileExists(pStr))
		map.image = LoadTexture(pStr);
	else{
		_noBackground = 1;
		map.image = LoadTexture("background.png");
	}
	
	
	

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
		if(strcmp(line, "[MusicFile]\n") == 0)		{mode = fpMusicFile;	continue;}
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
			case fpMusicFile:
				map.name = malloc(100);
				strcpy(map.musicFile, line);
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
		// _noBackground = 1;
	}
	return map;
}

void saveFile (int noteAmount)
{
	printf("written map data\n");
	fprintf(_pFile, "[Name]\n");
	fprintf(_pFile, "%s\n", _map->name);
	fprintf(_pFile, "[Creator]\n");
	fprintf(_pFile, "%s\n", _map->creator);
	fprintf(_pFile, "[Difficulty]\n");
	fprintf(_pFile, "%i\n", _map->difficulty);
	fprintf(_pFile, "[BPM]\n");
	fprintf(_pFile, "%i\n", _map->bpm);
	fprintf(_pFile, "[MusicFile]\n");
	fprintf(_pFile, "%s\n", _map->musicFile);
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
void loadMap (int fileType)
{
	char * map = malloc(strlen(_map->folder) + 12);
	strcpy(map, "maps/");
	strcat(map, _map->folder);
	char * pStr = malloc(strlen(map) + 12);
	_background = _map->image;

	strcpy(pStr, map);
	if(_map->musicFile == 0)
	{
		_map->musicFile = malloc(100);
		strcpy(_map->musicFile, "/song.mp3");
	}
	strcat(pStr, _map->musicFile);

	// ma_result result
	loadMusic(pStr);
	

	strcpy(pStr, map);
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
					break;
				case fpCreator:
					break;
				case fpDifficulty:
					break;
				case fpBPM:
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
	}else
	{
		_pFile = fopen(pStr, "wb");
	}
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
	while(fgets(line,sizeof(line),_pFile)!= NULL)
	{
		*score = atoi(line);
		char * part = &line[0];
		for(int i = 0; i < 1000; i++)
		{
			if(*part==' ');
				break;
			part++;
		}
		*combo = atoi(part);
	}
	fclose(file);
	return true;
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
	free(_pNotes);
	_amountNotes = 0;
	_noteIndex = 0;
}