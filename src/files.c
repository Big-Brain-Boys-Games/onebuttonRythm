
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
extern Note ** _papNotes;
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

void initFolders()
{
	if(!DirectoryExists("maps/"))
	{
		mkdir("maps");
	}

	if(!DirectoryExists("scores/"))
	{
		mkdir("scores");
	}
}


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
	printf("%s\n", pStr);
	if(!FileExists(pStr))
	{
		free(pStr);
		printf("map.data doesn't exist\n");
		return (Map){.name=0};
	}
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
		
		if(strcmp(line, "[ID]") == 0)					{mode = fpID;					continue;}
		if(strcmp(line, "[Name]") == 0)					{mode = fpName;					continue;}
		if(strcmp(line, "[Artist]") == 0)				{mode = fpArtist;				continue;}
		if(strcmp(line, "[Map Creator]") == 0)			{mode = fpMapCreator;			continue;}
		if(strcmp(line, "[Difficulty]") == 0)			{mode = fpDifficulty;			continue;}
		if(strcmp(line, "[BPM]") == 0)					{mode = fpBPM;					continue;}
		if(strcmp(line, "[Image]") == 0)				{mode = fpImage;				continue;}
		if(strcmp(line, "[MusicFile]") == 0)			{mode = fpMusicFile;			continue;}
		if(strcmp(line, "[MusicLength]") == 0)			{mode = fpMusicLength;			continue;}
		if(strcmp(line, "[MusicPreviewOffset]") == 0)	{mode = fpMusicPreviewOffset;	continue;}
		if(strcmp(line, "[Zoom]") == 0)					{mode = fpZoom;					continue;}
		if(strcmp(line, "[Offset]") == 0)				{mode = fpOffset;				continue;}
		if(strcmp(line, "[Beats]") == 0)				{mode = fpBeats;				continue;}
		if(strcmp(line, "[Notes]") == 0)				{break;}

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
			case fpMusicPreviewOffset:
				map.musicPreviewOffset = atof(line);
				break;
			case fpOffset:
				map.offset = atoi(line);
				break;
			case fpBeats:
				map.beats = atof(line);
				break;
			case fpNotes:
				//neat, notes :P
				//not needed for loadmapinfo, is used for loadmap
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
	// SetTextureFilter(map.image, TEXTURE_FILTER_BILINEAR);
	free(pStr);
	printf("successfully loaded %s\n", file);
	return map;
}

#define freeArray(arr) \
	if(arr) { \
		free(arr); \
		arr = 0; \
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
	freeArray(map->name);
	freeArray(map->artist);
	freeArray(map->mapCreator);
	freeArray(map->folder);
	freeArray(map->imageFile);
	freeArray(map->musicFile);
	freeArray(map->music);
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
	fprintf(_pFile, "[MusicPreviewOffset]\n");
	fprintf(_pFile, "%f\n", _map->musicPreviewOffset);
	fprintf(_pFile, "[Beats]\n");
	fprintf(_pFile, "%f\n", _map->beats);
	fprintf(_pFile, "[Notes]\n");
	
	for(int i = 0; i < noteAmount; i++)
	{
		if(_papNotes[i]->time == 0)
			continue;
		
		fprintf(_pFile, "%f", _papNotes[i]->time);
		if (_papNotes[i]->hitSE_File != 0)
			fprintf(_pFile, " \"%s\"", _papNotes[i]->hitSE_File);
		if (_papNotes[i]->texture_File != 0)
			fprintf(_pFile, " \"%s\"",_papNotes[i]->texture_File);
		if (_papNotes[i]->anim && _papNotes[i]->animSize)
		{
			fprintf(_pFile, " \"(%i",_papNotes[i]->animSize);
			for(int j = 0; j < _papNotes[i]->animSize; j++)
			{
				fprintf(_pFile, " %f,%f,%f", _papNotes[i]->anim[j].time, _papNotes[i]->anim[j].vec.x, _papNotes[i]->anim[j].vec.y);
			}
			fprintf(_pFile, ")\"");
		}
		if (_papNotes[i]->health > 0)
			fprintf(_pFile, " \"h%i\"", (int)(_papNotes[i]->health));
		
		fprintf(_pFile, "\n");
	}
	fclose(_pFile);
	
}

//custom note assets

//load all custom note textures & hit sounds
//todo, dynamic allocation
CustomTexture ** _paCustomTextures = 0;
int _customTexturesSize = 0;
CustomSound ** _paCustomSounds = 0;
int _customSoundsSize = 0;

CustomTexture * addCustomTexture(char * file)
{
	if(file == 0 || !strlen(file))
	{
		printf("addCustomTexture got empty file\n");
		return 0; // >:(
	}
	
	if(_customTexturesSize == 0)
	{
		_customTexturesSize++;
		_paCustomTextures = malloc(_customTexturesSize*sizeof(CustomTexture));
		_paCustomTextures[0] = malloc(sizeof(CustomTexture));
		_paCustomTextures[0]->file = malloc(strlen(file));
		strcpy(_paCustomTextures[0]->file, file);
		_paCustomTextures[0]->texture = LoadTexture(file);
		_paCustomTextures[0]->uses = 1;
		return _paCustomTextures[0];
	}else
	{
		int found = -1;
		for(int i = 0; i < _customTexturesSize; i++)
		{
			if(strcmp(_paCustomTextures[i]->file, file) == 0)
			{
				found = i;
				break;
			}
		}

		if(found == -1)
		{
			_paCustomTextures = realloc(_paCustomTextures, (_customTexturesSize+1) * sizeof(CustomTexture));
			_paCustomTextures[_customTexturesSize] = malloc(sizeof(CustomTexture));
			_paCustomTextures[_customTexturesSize]->file = file;
			_paCustomTextures[_customTexturesSize]->texture = LoadTexture(file);
			_paCustomTextures[_customTexturesSize]->uses = 1;
			_customTexturesSize++;
			return _paCustomTextures[_customTexturesSize-1];
		}else {
			_paCustomTextures[found]->uses++;
			return _paCustomTextures[found];
		}
	}
}

void removeCustomTexture(char * file)
{
	if(file == 0 || !strlen(file))
	{
		printf("removeCustomTexture got empty file\n");
		return; // >:(
	}

	int index = -1;
	for(int i = 0; i < _customTexturesSize; i++)
	{
		if(strcmp(_paCustomTextures[i]->file, file) == 0)
		{
			index = i;
			break;
		}
	}
	if(index == -1)
		return;

	_paCustomTextures[index]->uses--;
	printf("uses %s %i\n", file, _paCustomTextures[index]->uses);
	if(_paCustomTextures[index]->uses <= 0)
	{
		printf("no uses for it anymore, removing %s\n", file);
		//free texture
		UnloadTexture(_paCustomTextures[index]->texture);
		free(_paCustomTextures[index]->file);
		free(_paCustomTextures[index]);
		_customTexturesSize--;
		for(int i = index; i < _customTexturesSize; i++)
		{
			_paCustomTextures[i] = _paCustomTextures[i+1];
		}
		_paCustomTextures = realloc(_paCustomTextures, sizeof(CustomTexture)*_customTexturesSize);
	}
}

void freeAllCustomTextures ()
{
	if(_paCustomTextures)
	{
		for(int i = 0; i < _customTexturesSize; i++)
		{
			free(_paCustomTextures[i]->file);
			UnloadTexture(_paCustomTextures[i]->texture);
			free(_paCustomTextures[i]);
		}
		free(_paCustomTextures);
	}
	_customTexturesSize = 0;
	_paCustomTextures = 0;
}

CustomSound * addCustomSound(char * file)
{
	if(file == 0 || !strlen(file))
	{
		printf("addCustomSound got empty file\n");
		return 0; // >:(
	}
	
	if(_customSoundsSize == 0)
	{
		_customSoundsSize++;
		_paCustomSounds = malloc(_customSoundsSize*sizeof(CustomSound));
		_paCustomSounds[0] = malloc(sizeof(CustomSound));
		_paCustomSounds[0]->file = malloc(strlen(file));
		strcpy(_paCustomSounds[0]->file, file);
		loadAudio(&_paCustomSounds[0]->sound, file, &_paCustomSounds[0]->length);
		_paCustomSounds[0]->uses = 1;

		return _paCustomSounds[0];
	}else
	{
		int found = -1;
		for(int i = 0; i < _customSoundsSize; i++)
		{
			if(strcmp(_paCustomSounds[i]->file, file) == 0)
			{
				found = i;
				break;
			}
		}

		if(found == -1)
		{
			_paCustomSounds = realloc(_paCustomSounds, (_customSoundsSize+1) * sizeof(CustomSound));
			_paCustomSounds[_customSoundsSize] = malloc(sizeof(CustomSound));
			_paCustomSounds[_customSoundsSize]->file = file;
			loadAudio(&_paCustomSounds[found]->sound, file, &_paCustomSounds[found]->length);
			_paCustomSounds[_customSoundsSize]->uses = 1;
			_customSoundsSize++;
			return _paCustomSounds[_customSoundsSize-1];
		}else {
			_paCustomSounds[found]->uses++;
			return _paCustomSounds[found];
		}
	}
}

void removeCustomSound(char * file)
{
	if(file == 0 || !strlen(file))
	{
		printf("removeCustomSound got empty file\n");
		return; // >:(
	}

	int index = -1;
	for(int i = 0; i < _customSoundsSize; i++)
	{
		if(strcmp(_paCustomSounds[i]->file, file) == 0)
		{
			index = i;
			break;
		}
	}
	if(index == -1)
		return;

	_paCustomSounds[index]->uses--;
	printf("uses %s %i\n", file, _paCustomSounds[index]->uses);
	if(_paCustomSounds[index]->uses <= 0)
	{
		printf("no uses for it anymore, removing %s\n", file);
		//free music
		free(_paCustomSounds[index]->sound);
		free(_paCustomSounds[index]->file);
		free(_paCustomSounds[index]);
		_customSoundsSize--;
		for(int i = index; i < _customSoundsSize; i++)
		{
			_paCustomSounds[i] = _paCustomSounds[i+1];
		}
		_paCustomSounds = realloc(_paCustomSounds, sizeof(CustomTexture)*_customSoundsSize);
	}
}

void freeAllCustomSounds ()
{
	if(_paCustomSounds)
	{
		for(int i = 0; i < _customSoundsSize; i++)
		{
			free(_paCustomSounds[i]->file);
			free(_paCustomSounds[i]->sound);
			free(_paCustomSounds[i]);
		}
		free(_paCustomSounds);
	}
	_customSoundsSize = 0;
	_paCustomSounds = 0;
}


//Load the map. Use 1 parameter to only open the file
//TODO Change to loadNotes();
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

	freeNotes();
	freeAllCustomSounds();
	freeAllCustomTextures();


	//text file
	char line [1000];
	enum FilePart mode = fpNone;
	_amountNotes = 50;
	_papNotes = malloc(_amountNotes*sizeof(Note*));
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
					_papNotes = realloc(_papNotes, _amountNotes*sizeof(Note*));
				}
				_papNotes[_noteIndex] = malloc(sizeof(Note));
				_papNotes[_noteIndex]->time = atof(line);
				_papNotes[_noteIndex]->hitSE_File = 0;
				_papNotes[_noteIndex]->texture_File = 0;
				_papNotes[_noteIndex]->anim = 0;
				_papNotes[_noteIndex]->animSize = 0;
				_papNotes[_noteIndex]->custSound = 0;
				_papNotes[_noteIndex]->custTex = 0;
				_papNotes[_noteIndex]->health = 1;
				_papNotes[_noteIndex]->hit = 0;

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
					char tmpStr[100] = {0};
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
					//Loading files
					char * ext = GetFileExtension(tmpStr);
					if(ext != NULL)
					{
						if(strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0)
						{
							printf("found sound %s\n", tmpStr);
							//found hit sound
							char fullPath [100];
							snprintf(fullPath, 100, "maps/%s/%s", _map->folder, tmpStr);
							_papNotes[_noteIndex]->custSound = addCustomSound(fullPath);
							_papNotes[_noteIndex]->hitSE_File = malloc(100*sizeof(char));
							strcpy(_papNotes[_noteIndex]->hitSE_File, tmpStr);
						}else if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png")  == 0 || strcmp(ext, ".jpeg") == 0)
						{
							printf("found texture %s\n", tmpStr);
							//found texture
							char fileStr [50];
							for(int k = 0; k < 50; k++)
							{
								if(tmpStr[k] == '"')
								{
									fileStr[k] = '\0';
									break;
								}
								fileStr[k] = tmpStr[k];
							}
							char fullPath [100];
							snprintf(fullPath, 100, "maps/%s/%s", _map->folder, fileStr);
							_papNotes[_noteIndex]->custTex = addCustomTexture(fullPath); 
							_papNotes[_noteIndex]->texture_File = malloc(100*sizeof(char));
							strcpy(_papNotes[_noteIndex]->texture_File, tmpStr);
						}
					}
					//Loading animation
					else if(line[part] == '(')
					{
						//found animation
						_papNotes[_noteIndex]->animSize = atoi(&(line[part+1]));
						_papNotes[_noteIndex]->anim = calloc(_papNotes[_noteIndex]->animSize, sizeof(Frame));
						while(line[part] != ' ')
								part++;
						part++;
						printf("found animation %i  ", _papNotes[_noteIndex]->animSize);
						for(int i = 0; i < _papNotes[_noteIndex]->animSize; i++)
						{
							part++; //skip space
							_papNotes[_noteIndex]->anim[i].time = atof(&(line[part]));
							while(line[part] != ',')
								part++;
							part++;
							_papNotes[_noteIndex]->anim[i].vec.x = atof(&(line[part]));
							while(line[part] != ',')
								part++;
							part++;
							_papNotes[_noteIndex]->anim[i].vec.y = atof(&(line[part]));
							while(line[part] != ' ')
								part++;
							printf("%f  %f  %f    ", _papNotes[_noteIndex]->anim[i].time, _papNotes[_noteIndex]->anim[i].vec.x, _papNotes[_noteIndex]->anim[i].vec.y);
						}
						printf("\n");
					}
					else if (line[part] == 'h')
					{
						part++;
						//found health
						_papNotes[_noteIndex]->health = atof(&(line[part]));
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
	_noteIndex = 0;
	char str [100];
	snprintf(str, 100, "%s - %s", _map->name, _map->artist);
	SetWindowTitle(str);
}

void freeNotes()
{
	if(_papNotes)
	{
		int amountTex = 0;
		for(int i = 0; i < _amountNotes; i++)
		{
			freeArray(_papNotes[i]->anim);
			freeArray(_papNotes[i]->hitSE_File);
			freeArray(_papNotes[i]->texture_File);
			free(_papNotes[i]);
		}

		free(_papNotes);
	}
}

void saveScore()
{
	FILE * file;
	char str [100];
	snprintf(str, 100, "scores/%s", _map->name);
	if(!DirectoryExists(str))
		mkdir(str);
	snprintf(str, 100, "scores/%s/%s", _map->name, _playerName);
	printf("str %s\n", str);
	file = fopen(str, "w");
	fprintf(file, "%i %i %f", _score, _highestCombo, _averageAccuracy);
	fclose(file);
}

bool readScore(Map * map, int *score, int * combo, float * accuracy)
{
	*score = 0;
	*combo = 0;
	FILE * file;
	char str [100];
	snprintf(str, 100, "scores/%s/%s", map->name, _playerName);
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
	// free(_papNotes);
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