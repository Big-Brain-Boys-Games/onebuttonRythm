
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../deps/raylib/src/raylib.h"
#include "windowsDefs.h"
#include "../deps/zip/src/zip.h"


#define EXTERN_DRAWING
#define EXTERN_GAMEPLAY
#define EXTERN_EDITOR
#define EXTERN_AUDIO
#define EXTERN_MAIN
#define EXTERN_MENUS

#include "audio.h"
#include "files.h"
#include "drawing.h"
#include "main.h"

#include "gameplay/gameplay.h"
#include "gameplay/menus.h"
#include "gameplay/editor.h"



#ifdef __unix
#include <sys/stat.h>
#define mkdir(dir) mkdir(dir, 0777)
#endif

Map * _paMaps = 0;

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
	char * mapStr = malloc(100);
	// strncpy(mapStr, "maps/");
	strncpy(mapStr, file, 100);
	Map map = {0};
	map.zoom = 7;
	map.offset = 0;
	map.folder = malloc(100);
	map.musicLength = 0;
	map.musicFile = 0;
	map.music.size = 0;
	map.music.data = 0;
	map.beats = 4;
	strncpy(map.folder, file, 100);
	char * pStr = malloc(100);
	map.imageFile = malloc(100);

	snprintf(pStr, 100, "%s/map.data", mapStr);
	if(!FileExists(pStr))
	{
		free(pStr);
		printf("map.data doesn't exist\n");
		return (Map){0};
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
		int stringLength = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLength; i++)
			if(line[i] != ' ' && line[i] != '\n' && line[i] != '\r')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		for(int i = 0; i < 100; i++)
			if(line[i] == '\n' || line[i] == '\r' || line[i] == 0)
				line[i] = '\0';
		
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
		if(strcmp(line, "[Notes]") == 0)				{break;} //Notes is at the end of map.data so we can just skip everything else after

		switch(mode)
		{
			case fpNone:
				break;
			case fpID:
				map.id = atoi(line);
			case fpName:
				strncpy(map.name, line, 100);
				break;
			case fpArtist:
				strncpy(map.artist, line, 100);
				break;
			case fpMapCreator:
				strncpy(map.mapCreator, line, 100);
				break;
			case fpDifficulty:
				map.difficulty = atoi(line);
				break;
			case fpBPM:
				map.bpm = atoi(line);
				break;
			case fpImage:
				strncpy(map.imageFile, line, 100);
				break;
			case fpMusicFile:
				strncpy(map.musicFile, line, 100);
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
				map.beats = atoi(line);
				if(map.beats < 1)
					map.beats = 4; //setting it to a sensable default (0 crashes game)
				break;
			case fpTimeSignatures:
			case fpNotes:
				//neat, notes :P
				//not needed for loadmapinfo, is used for loadmap
				break;
		}
	}
	fclose(f);
	
	if(map.imageFile[0] == '\0')
	{
		strncpy(map.imageFile, "/image.png", 100);
	}

	snprintf(pStr, 100, "%s%s", mapStr, map.imageFile);

	if(FileExists(pStr))
	{
		map.cpuImage.width = 0;
	}
	else{
		map.image = _menuBackground;
		map.cpuImage.width = -1;
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
	freeArray(map->music.data);
	map->music.size = 0;
	map->bpm = 0;
	map->difficulty = 0;
	map->zoom = 7;
	map->offset = 0;
	map->musicLength = 0;
	map->id = 0;
}

void saveMap ()
{
	char str [100];
	snprintf(str, 100, "%s/map.data", _map->folder);
	FILE * pFile = fopen(str, "w");
	printf("written map data\n");
	fprintf(pFile, "[ID]\n");
	fprintf(pFile, "%i\n", _map->id);
	fprintf(pFile, "[Name]\n");
	fprintf(pFile, "%s\n", _map->name);
	fprintf(pFile, "[Artist]\n");
	fprintf(pFile, "%s\n", _map->artist);
	fprintf(pFile, "[Map Creator]\n");
	fprintf(pFile, "%s\n", _map->mapCreator);
	fprintf(pFile, "[Difficulty]\n");
	fprintf(pFile, "%i\n", _map->difficulty);
	fprintf(pFile, "[BPM]\n");
	fprintf(pFile, "%i\n", _map->bpm);
	if(_map->imageFile != 0)
	{
		fprintf(pFile, "[Image]\n");
		fprintf(pFile, "%s\n", _map->imageFile);
	}
	fprintf(pFile, "[MusicFile]\n");
	fprintf(pFile, "%s\n", _map->musicFile);
	fprintf(pFile, "[MusicLength]\n");
	if(!_map->musicLength && _pMusic)
	{
		_map->musicLength = getMusicDuration();
	}
	fprintf(pFile, "%i\n", _map->musicLength);
	fprintf(pFile, "[Zoom]\n");
	fprintf(pFile, "%i\n", _map->zoom);
	fprintf(pFile, "[Offset]\n");
	fprintf(pFile, "%i\n", _map->offset);
	fprintf(pFile, "[MusicPreviewOffset]\n");
	fprintf(pFile, "%f\n", _map->musicPreviewOffset);
	fprintf(pFile, "[Beats]\n");
	fprintf(pFile, "%d\n", _map->beats);

	fprintf(pFile, "[TimeSignatures]\n");
	
	for(int i = 0; i < _amountTimingSegments; i++)
	{
		fprintf(pFile, "%f %i %i\n", _paTimingSegment[i].time, _paTimingSegment[i].bpm, _paTimingSegment[i].beats);
	}

	fprintf(pFile, "[Notes]\n");
	
	for(int i = 0; i < _amountNotes; i++)
	{
		if(_papNotes[i]->time == 0)
			continue;
		
		fprintf(pFile, "%f", _papNotes[i]->time);
		if (_papNotes[i]->hitSE_File != 0)
			fprintf(pFile, " \"%s\"", _papNotes[i]->hitSE_File);

		if (_papNotes[i]->texture_File != 0)
			fprintf(pFile, " \"%s\"",_papNotes[i]->texture_File);

		if (_papNotes[i]->anim && _papNotes[i]->animSize)
		{
			fprintf(pFile, " \"(%i",_papNotes[i]->animSize);
			for(int j = 0; j < _papNotes[i]->animSize; j++)
			{
				fprintf(pFile, " %f,%f,%f", _papNotes[i]->anim[j].time, _papNotes[i]->anim[j].vec.x, _papNotes[i]->anim[j].vec.y);
			}
			fprintf(pFile, ")\"");
		}

		if (_papNotes[i]->health > 0)
			fprintf(pFile, " \"h%i\"", (int)(_papNotes[i]->health));
		
		fprintf(pFile, "\n");
	}
	fclose(pFile);
	
}

//custom note assets

//load all custom note textures & hit sounds
CustomTexture ** _paCustomTextures = 0;
int _customTexturesSize = 0;
CustomSound ** _paCustomSounds = 0;
int _customSoundsSize = 0;

CustomTexture * addCustomTexture(char * file)
{
	if(file == 0 || !strlen(file) || !FileExists(file))
	{
		printf("addCustomTexture got empty file\n");
		return 0; // >:(
	}
	
	if(_customTexturesSize == 0)
	{
		_customTexturesSize++;
		_paCustomTextures = malloc(_customTexturesSize*sizeof(CustomTexture));
		_paCustomTextures[0] = malloc(sizeof(CustomTexture));
		_paCustomTextures[0]->file = malloc(100);
		strncpy(_paCustomTextures[0]->file, file, 100);
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
	if(_paCustomTextures[index]->uses <= 0)
	{
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
	if(file == 0 || !strlen(file) || !FileExists(file))
	{
		printf("addCustomSound got non existant file\n");
		return 0; // >:(
	}
	
	if(_customSoundsSize == 0)
	{
		_customSoundsSize++;
		_paCustomSounds = malloc(_customSoundsSize*sizeof(CustomSound));
		_paCustomSounds[0] = malloc(sizeof(CustomSound));
		_paCustomSounds[0]->file = malloc(100);
		strncpy(_paCustomSounds[0]->file, file, 100);
		loadAudio(&_paCustomSounds[0]->sound, file);
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
			loadAudio(&_paCustomSounds[found]->sound, file);
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
	if(_paCustomSounds[index]->uses <= 0)
	{
		//free music
		freeArray(_paCustomSounds[index]->sound.data);
		freeArray(_paCustomSounds[index]->file);
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
			freeArray(_paCustomSounds[i]->file);
			freeArray(_paCustomSounds[i]->sound.data);
			free(_paCustomSounds[i]);
		}
		free(_paCustomSounds);
	}
	_customSoundsSize = 0;
	_paCustomSounds = 0;
}

void loadMap ()
{
	char * map = malloc(100);
	// strncpy(map, "maps/");
	strncpy(map, _map->folder, 100);
	char * pStr = malloc(100);

	char str[100];

	strncpy(pStr, map, 100);
	if(_map->musicFile == 0)
	{
		_map->musicFile = malloc(100);
		strncpy(_map->musicFile, "/song.mp3", 100);
	}

	if(_map->imageFile == 0)
	{
		_map->imageFile = malloc(100);
		strncpy(_map->imageFile, "/image.png", 100);
	}

	snprintf(str, 100, "%s/%s", _map->folder, _map->imageFile);
	if(!_map->image.id)
		_map->image = LoadTexture(str);
	
	if(_map->image.id)
	{
		_background = _map->image;
		_noBackground = false;
	}else{
		_map->image = _menuBackground;
	}


	if(_map->image.id == _menuBackground.id)
		_noBackground = true;

	if(_map->music.data==0 || _map->music.size == 0)
	{
		char str[100];
		snprintf(str, 100, "%s/%s", _map->folder, _map->musicFile);
		loadMusic(&_map->music, str, _map->musicPreviewOffset);
	}
	
	_pMusic = &_map->music;
	// _map->musicLength = (int)getMusicDuration();

	snprintf(pStr, 100, "%s/map.data", map);
	FILE * pFile = fopen(pStr, "r");

	freeNotes();
	freeAllCustomSounds();
	freeAllCustomTextures();


	//text file
	char line [1000];
	enum FilePart mode = fpNone;
	_amountNotes = 50;
	_papNotes = malloc(_amountNotes*sizeof(Note*));
	_noteIndex = 0;


	while(fgets(line,sizeof(line),pFile)!= NULL)
	{
		int stringLength = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLength; i++)
			if(line[i] != ' ' && line[i] != '\n' && line[i] != '\r')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		for(int i = 0; i < stringLength; i++)
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
		if(strcmp(line, "[TimeSignatures]") == 0)		{mode = fpTimeSignatures;		continue;}
		if(strcmp(line, "[Notes]") == 0)				{mode = fpNotes;				continue;}

		switch(mode)
		{
			default:
				break;
			case fpTimeSignatures:
				if(!_paTimingSegment)
				{
					_paTimingSegment = malloc(sizeof(TimingSegment));
					_amountTimingSegments = 1;
				}
				else
				{
					_amountTimingSegments++;
					_paTimingSegment = realloc(_paTimingSegment, sizeof(TimingSegment) * _amountTimingSegments);
				}

				_paTimingSegment[_amountTimingSegments-1].time = atof(line);

				char * partLine = &(line[0]);

				//skip to ' '
				for(;*partLine != ' ' && *partLine != '\0'; partLine++)
				{ }

				_paTimingSegment[_amountTimingSegments-1].bpm = atoi(partLine);

				if(*partLine != '\0') partLine++;

				//skip to ' '
				for(;*partLine != ' ' && *partLine != '\0'; partLine++)
				{ }

				_paTimingSegment[_amountTimingSegments-1].beats = fmin(atoi(partLine), 32);
				break;


			case fpNotes:
				double time = atof(line);

				int index = _noteIndex - 1;
				if(index > 0 && _papNotes[index]->time > time)
				{
					//note out of order, skip note
					continue;
				}

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

				index = 0;
				for(int j = 0; j < 2; j++)
				{
					//find " or end of line
					for(int i = index+1; i < 1000; i++)
					{
						if(line[i] == '\r' || line[i] == '\n' || line[i] == '\0' || line[i] == 0)
						{
							index = -1;
							break;
						}
						if(line[i] == '"')
						{
							index = i+1;
							break;
						}
					}

					if(index == -1)
						break;
					

					char tmpStr[100] = {0};
					int strPos = 0;
					for(int i = 0; i < 100; i++)
					{
						if(line[i+index] == '"' || line[i+index] == '\0')
						{
							tmpStr[strPos] = '\0';
							break;
						}
						tmpStr[strPos] = line[i+index];
						strPos++;
					}

					char * ext = (char *) GetFileExtension(tmpStr);

					//Loading files
					if(ext != NULL)
					{
						if(strcmp(ext, ".mp3") == 0 || strcmp(ext, ".wav") == 0)
						{
							printf("found sound %s\n", tmpStr);
							//found hit sound
							char fullPath [100];
							snprintf(fullPath, 100, "%s/%s", _map->folder, tmpStr);
							_papNotes[_noteIndex]->custSound = addCustomSound(fullPath);
							_papNotes[_noteIndex]->hitSE_File = malloc(100*sizeof(char));
							strncpy(_papNotes[_noteIndex]->hitSE_File, tmpStr, 100);
						}else if(strcmp(ext, ".jpg") == 0 || strcmp(ext, ".png")  == 0 || strcmp(ext, ".jpeg") == 0)
						{
							printf("found texture %s\n", tmpStr);
							//found texture
							char fileStr [100];
							for(int k = 0; k < 100; k++)
							{
								if(tmpStr[k] == '"')
								{
									fileStr[k] = '\0';
									break;
								}
								fileStr[k] = tmpStr[k];
							}
							char fullPath [100];
							snprintf(fullPath, 100, "%s/%s", _map->folder, fileStr);
							_papNotes[_noteIndex]->custTex = addCustomTexture(fullPath); 
							_papNotes[_noteIndex]->texture_File = malloc(100*sizeof(char));
							strncpy(_papNotes[_noteIndex]->texture_File, tmpStr, 100);
						}
					}
					//Loading animation
					if(line[index] == '(')
					{
						//found animation
						_papNotes[_noteIndex]->animSize = atoi(&(line[index+1]));
						_papNotes[_noteIndex]->anim = calloc(_papNotes[_noteIndex]->animSize, sizeof(Frame));

						//skip to first frame
						while(line[index] != ' ')
								index++;

						index++; //skip space

						for(int i = 0; i < _papNotes[_noteIndex]->animSize; i++)
						{
							_papNotes[_noteIndex]->anim[i].time = atof(&(line[index]));
							
							while(line[index] != ',')
								index++;
							index++;

							_papNotes[_noteIndex]->anim[i].vec.x = atof(&(line[index]));

							while(line[index] != ',')
								index++;
							index++;

							_papNotes[_noteIndex]->anim[i].vec.y = atof(&(line[index]));

							while(line[index] != ' ')
								index++;
							index++;
						}
					}
					else if (line[index] == 'h')
					{
						index++;
						//found health
						_papNotes[_noteIndex]->health = atof(&(line[index]));
					}

					// free(ext);

					//go to end of part
					for(int i = index; i < 1000; i++)
					{
						if(line[i] == '"')
						{
							index = i+1;
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
	fclose(pFile);
	free(pStr);
	snprintf(str, 100, "%s - %s", _map->name, _map->artist);
	SetWindowTitle(str);	
}

void freeNote(Note * note)
{
	if(note->hitSE_File) removeCustomSound(note->hitSE_File);
	if(note->texture_File) removeCustomTexture(note->texture_File);
	freeArray(note->anim);
	freeArray(note->hitSE_File);
	freeArray(note->texture_File);
	// free(note);
}

void freeNotes()
{
	if(_papNotes)
	{
		for(int i = 0; i < _amountNotes; i++)
		{
			freeNote(_papNotes[i]);
			free(_papNotes[i]);
		}

		free(_papNotes);
		_papNotes = 0;
	}
	if(_paTimingSegment)
	{
		free(_paTimingSegment);
		_paTimingSegment = 0;
		_amountTimingSegments = 0;
	}
}

void saveScore()
{
	FILE * file;
	char str [200];
	snprintf(str, 200, "scores/%s", _map->name);
	if(!DirectoryExists(str))
		mkdir(str);
	
	snprintf(str, 200, "scores/%s/%s", _map->name, _playerName);
	file = fopen(str, "w");
	fprintf(file, "%i %i %f %i %i", _score, _highestCombo, _averageAccuracy, _notesMissed,
		rankCalculation(_score, _highestCombo, _notesMissed, _averageAccuracy));
	fclose(file);
}

void readScore(Map * map, int *score, int * combo, int * misses, float * accuracy, int * rank)
{
	*score = 0;
	*combo = 0;
	*accuracy = 0;
	*misses = 0;
	*rank = 0;

	FILE * file;
	char str [200];
	snprintf(str, 200, "scores/%s/%s", map->name, _playerName);

	if(!DirectoryExists("scores/"))
		return;
	
	if(!FileExists(str))
		return;
	
	file = fopen(str, "r");
	char line [1000];
	while(fgets(line,sizeof(line),file)!= NULL)
	{
		*score = atoi(line);
		char * part = &line[0];
		for(int i = 0; i < 1000; i++)
		{
			if(*part == ' ')
			{
				part++;
				break;
			}
			if(*part == '\0')
				return;
			part++;
		}
		*combo = atoi(part);
		for(int i = 0; i < 1000; i++)
		{
			if(*part == ' ')
			{
				part++;
				break;
			}
			if(*part == '\0')
				return;
			part++;
		}
		*accuracy = atof(part);
		for(int i = 0; i < 1000; i++)
		{
			if(*part == ' ')
			{
				part++;
				break;
			}
			if(*part == '\0')
				return;
			part++;
		}
		*misses = atoi(part);
		for(int i = 0; i < 1000; i++)
		{
			if(*part == ' ')
			{
				part++;
				break;
			}
			if(*part == '\0')
				return;
			part++;
		}
		*rank = atoi(part);
	}
	fclose(file);
	return;
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

	snprintf(str, 100, "%s.zip", map->name);
	struct zip_t *zip = zip_open(str, 6, 'w');
	strncpy(str, map->folder, 100);
	FilePathList files = LoadDirectoryFiles(str);
	for(int i = 0; i < files.count; i++)
	{
		if(files.paths[i][0] == '.')
			continue;
		zip_entry_open(zip, files.paths[i]);
		{
			snprintf(str, 100, "%s/%s", map->folder, files.paths[i]);

			FILE * file = fopen(files.paths[i], "r");
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
	UnloadDirectoryFiles(files);
}

void addZipMap(char * file)
{
	int arg = 2;
	char str [100];
	snprintf(str, 100, "maps/%s", GetFileNameWithoutExt(file));
	zip_extract(file, str, on_extract_entry, &arg);
	_mapRefresh = true;
}

void makeMap(Map * map)
{
	if(!map || !map->name)
		return;
	
	char str [100];
	snprintf(str, 100, "maps/%s", map->name);
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
		int stringLength = strlen(line);
		bool emptyLine = true;
		for(int i = 0; i < stringLength; i++)
			if(line[i] != ' ' && line[i] != '\n')
				emptyLine = false;
		
		if(emptyLine)
			continue;

		if(strcmp(line, "[Name]\n") == 0)					{mode = spName;			continue;}
		if(strcmp(line, "[Volume Global]\n") == 0)			{mode = spVolGlobal;	continue;}
		if(strcmp(line, "[Volume Music]\n") == 0)			{mode = spVolMusic;		continue;}
		if(strcmp(line, "[Volume Sound Effects]\n") == 0)	{mode = spVolSE;		continue;}
		if(strcmp(line, "[Zoom]\n") == 0)					{mode = spZoom;			continue;}
		if(strcmp(line, "[NoteSize]\n") == 0)				{mode = spNoteSize;		continue;}
		if(strcmp(line, "[Offset]\n") == 0)					{mode = spOffset;		continue;}
		if(strcmp(line, "[ResolutionX]\n") == 0)			{mode = spResX;			continue;}
		if(strcmp(line, "[ResolutionY]\n") == 0)			{mode = spResY;			continue;}
		if(strcmp(line, "[Fullscreen]\n") == 0)				{mode = spFS;			continue;}
		if(strcmp(line, "[Animations]\n") == 0)				{mode = spAnimations;	continue;}
		if(strcmp(line, "[CustomAssets]\n") == 0)			{mode = spCustomAssets;	continue;}
		
		for(int i = 0; i < stringLength; i++)
					if(line[i] == '\n') line[i]= '\0';
		
		switch(mode)
		{
			case spNone:
				break;
			case spName:
				strncpy(_playerName, line, 100);
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
			case spNoteSize:
				_settings.noteSize = atoi(line);
				break;
			case spOffset:
				_settings.offset = atof(line);
				break;
			case spResX:
				_settings.resolutionX = atoi(line);
				break;
			case spResY:
				_settings.resolutionY = atoi(line);
				break;
			case spFS:
				_settings.fullscreen = atoi(line);
				break;
			case spAnimations:
				_settings.animations = atoi(line);
				break;
			case spCustomAssets:
				_settings.customAssets = atoi(line);
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
	fprintf(file, "[NoteSize]\n");
	fprintf(file, "%i\n", _settings.noteSize);
	fprintf(file, "[Offset]\n");
	fprintf(file, "%f\n", _settings.offset);
	fprintf(file, "[ResolutionX]\n");
	fprintf(file, "%i\n", _settings.resolutionX);
	fprintf(file, "[ResolutionY]\n");
	fprintf(file, "%i\n", _settings.resolutionY);
	fprintf(file, "[Fullscreen]\n");
	fprintf(file, "%i\n", _settings.fullscreen);
	fprintf(file, "[Animations]\n");
	fprintf(file, "%i\n", _settings.animations);
	fprintf(file, "[CustomAssets]\n");
	fprintf(file, "%i\n", _settings.customAssets);
	fclose(file);
}