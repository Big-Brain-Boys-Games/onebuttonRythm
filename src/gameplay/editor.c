#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define EXTERN_FILES
#define EXTERN_AUDIO
#define EXTERN_GAMEPLAY
#define EXTERN_MAIN
#define EXTERN_DRAWING


#include "editor.h"

#include "playing.h"
#include "menus.h"
#include "gameplay.h"

#include "../files.h"
#include "../drawing.h"
#include "../audio.h"
#include "../main.h"



TimingSegment * _paTimingSegment = 0;
int _amountTimingSegments = 0;

Note **_selectedNotes = 0;
int _amountSelectedNotes = 0;
int _barMeasureCount = 2;


TimingSegment getTimingSignature(float time)
{
	if(!_paTimingSegment || _amountTimingSegments == 0)
		return (TimingSegment){.bpm=_map->bpm, .time=_map->offset/1000.0};
	for(int i = 0; i < _amountTimingSegments; i++)
	{
		if(time < _paTimingSegment[i].time)
		{
			i--;
			if(i < 0)
				return (TimingSegment){.bpm=_map->bpm, .time=_map->offset/1000.0};
			return _paTimingSegment[i];
		}
	}
	return _paTimingSegment[_amountTimingSegments-1];
}

TimingSegment * getTimingSignaturePointer(float time)
{
	if(!_paTimingSegment || _amountTimingSegments == 0)
		return 0;
	for(int i = 0; i < _amountTimingSegments; i++)
	{
		if(time < _paTimingSegment[i].time)
		{
			i--;
			if(i < 0)
				return 0;
			return &(_paTimingSegment[i]);
		}
	}
	return &(_paTimingSegment[_amountTimingSegments-1]);
}

TimingSegment * addTimingSignature(float time, int bpm)
{
	if(!_paTimingSegment)
	{
		_paTimingSegment = malloc(sizeof(TimingSegment));
		_amountTimingSegments = 1;
		_paTimingSegment[0].time = time;
		_paTimingSegment[0].bpm = bpm;
		return &(_paTimingSegment[0]);
	}
	for(int i = 0; i < _amountTimingSegments; i++)
	{
		if(time < _paTimingSegment[i].time)
		{
			i--;
			if(i < 0)
				i = 0;


			_amountTimingSegments++;
			_paTimingSegment = realloc(_paTimingSegment, _amountTimingSegments*sizeof(TimingSegment) );
			for(int j = _amountTimingSegments-2; j >= i; j--)
			{
				_paTimingSegment[j+1] = _paTimingSegment[j];
			}
			_paTimingSegment[i].time = time;
			_paTimingSegment[i].bpm = bpm;
			_paTimingSegment[i].beats = 4;
			return &(_paTimingSegment[i]);
		}
	}


	int  i = _amountTimingSegments;

	_amountTimingSegments++;
	_paTimingSegment = realloc(_paTimingSegment, _amountTimingSegments*sizeof(TimingSegment) );
	for(int j = _amountTimingSegments-2; j >= i; j--)
	{
		_paTimingSegment[j+1] = _paTimingSegment[j];
	}
	_paTimingSegment[i].time = time;
	_paTimingSegment[i].bpm = bpm;
	_paTimingSegment[i].beats = 4;
	return &(_paTimingSegment[i]);
}

void removeTimingSignature(float time)
{
	if(!_paTimingSegment)
		return;

	if(_amountTimingSegments == 1)
	{
		free(_paTimingSegment);
		_paTimingSegment = 0;
		_amountTimingSegments = 0;
		return;
	}
	for(int i = 0; i < _amountTimingSegments; i++)
	{
		if(time < _paTimingSegment[i].time)
		{
			i--;
			if(i < 0)
				i = 0;

			_amountTimingSegments--;
			_paTimingSegment = realloc(_paTimingSegment, _amountTimingSegments*sizeof(TimingSegment) );
			for(int j = i; j < _amountTimingSegments; j++)
			{
				_paTimingSegment[j] = _paTimingSegment[j+1];
			}
			return;
		}
	}
}


typedef enum CommandType{ComAdd, ComRemove, ComChangeNote} CommandType;
typedef struct Commmand{
	CommandType type;
	int cost;
	float time;
	Note data;
} Command;
#define COMMANDBUFFER 50
Command _aCommandBuffer[50] = {0};
int _CommandIndex = 0;
int _CommandFurtestIndex = 0;

#define freeArray(arr) \
	if(arr)\
		free(arr);\
	arr = 0;

void undoCommand(Command command)
{
	int index = 0;
	switch(command.type)
	{
		case ComAdd:
			removeNote(findClosestNote(_papNotes, _amountNotes, command.time));
			break;
		
		case ComRemove:
			index = newNote(command.time);
			*(_papNotes[index]) = command.data;
			break;

		case ComChangeNote:
			index = findClosestNote(_papNotes, _amountNotes, command.time);
			freeNote(_papNotes[index]);
			MakeNoteCopy(command.data, _papNotes[index]);
			break;

	}
}

void freeCommand(int index)
{
	freeNote(&_aCommandBuffer[index].data);
}

void undo()
{
	printf("undo\n");
	free(_selectedNotes);
	_selectedNotes = 0;
	_amountSelectedNotes = 0;

	while(_CommandIndex > 0 && _CommandIndex > _CommandFurtestIndex - COMMANDBUFFER)
	{
		int index = _CommandIndex % COMMANDBUFFER;
		int cost = _aCommandBuffer[index].cost;

		undoCommand(_aCommandBuffer[index]);
		freeCommand(index);

		_CommandIndex--;

		if(cost != 0)
			break;
	}

}

void doAction(CommandType type, int note, int cost)
{
	_CommandIndex++;
	if(_CommandIndex > _CommandFurtestIndex)
		_CommandFurtestIndex = _CommandIndex;
	int index = _CommandIndex % COMMANDBUFFER;
	_aCommandBuffer[index].type = type;
	_aCommandBuffer[index].cost = cost;
	_aCommandBuffer[index].time = _papNotes[note]->time;
	_aCommandBuffer[index].data = (Note) {0};

	switch(type)
	{
		case ComAdd:
			//called after note created
			break;

		case ComRemove:
			//called before note removed
			MakeNoteCopy(*_papNotes[note], &_aCommandBuffer[index].data);
			break;
		
		case ComChangeNote:
			//called before note changes
			MakeNoteCopy(*_papNotes[note], &_aCommandBuffer[index].data);
			break;
	}
}


void removeNote(int index)
{
	for(int i = 0; i < _amountSelectedNotes; i++)
		if(_selectedNotes[i] == _papNotes[index])
		{
			removeSelectedNote(i);
			break;
		}
	
	_amountNotes--;
	freeNote(_papNotes[index]);
	free(_papNotes[index]);

	for (int i = index; i < _amountNotes; i++)
	{
		_papNotes[i] = _papNotes[i + 1];
	}
	
	_papNotes = realloc(_papNotes, (_amountNotes+1) * sizeof(Note*));
}

int newNote(float time)
{
	int closestIndex = 0;
	_amountNotes++;
	_papNotes = realloc(_papNotes, (_amountNotes+1)* sizeof(Note*));
	for (int i = 0; i < _amountNotes - 1; i++)
	{
		_papNotes[i] = _papNotes[i];
		if (_papNotes[i]->time < time)
		{
			closestIndex = i + 1;
		}else break;
	}
	for (int i = _amountNotes-2; i >= closestIndex; i--)
	{
		_papNotes[i + 1] = _papNotes[i];
	}
	_papNotes[closestIndex] = calloc(1, sizeof(Note));
	_papNotes[closestIndex]->time = time;
	_papNotes[closestIndex]->health = 1;
	_papNotes[closestIndex]->hit = 0;
	return closestIndex;
}

void addSelectNote(int note)
{
	if(note == -1)
		return;
	for (int i = 0; i < _amountSelectedNotes; i++)
	{
		if (_selectedNotes[i] == _papNotes[note])
		{
			removeSelectedNote(i);
			return;
		}
	}

	_amountSelectedNotes++;
	if (_selectedNotes == 0)
		_selectedNotes = malloc(sizeof(Note*) * _amountSelectedNotes);
	else
		_selectedNotes = realloc(_selectedNotes, sizeof(Note*) * _amountSelectedNotes);
	_selectedNotes[_amountSelectedNotes - 1] = _papNotes[note];
}

void removeSelectedNote(int index)
{
	for (int i = index; i < _amountSelectedNotes-1; i++)
	{
		_selectedNotes[i] = _selectedNotes[i + 1];
	}
	_amountSelectedNotes--;
	_selectedNotes = realloc(_selectedNotes, sizeof(Note *) * _amountSelectedNotes);
}



void fEditorSongSettings(bool reset)
{
	ClearBackground(BLACK);
	drawBackground();
	drawVignette();

	// Darken background
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});
	// BPM setting
	char bpm[10] = {0};
	if (_map->bpm != 0)
		snprintf(bpm, 10, "%i", _map->bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.1, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(bpmBox, bpm, &bpmBoxSelected);
	_map->bpm = atoi(bpm);
	_map->bpm = fmin(fmax(_map->bpm, 0), 300);

	// Offset setting
	char offset[10] = {0};
	if (_map->offset != 0)
		snprintf(offset, 10, "%i", _map->offset);
	static bool offsetBoxSelected = false;
	Rectangle offsetBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.18, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(offsetBox, offset, &offsetBoxSelected);
	_map->offset = atoi(offset);
	_map->offset = fmin(fmax(_map->offset, 0), 5000);

	// song name setting
	char songName[50] = {0};
	snprintf(songName, 50, "%s", _map->name);
	static bool songNameBoxSelected = false;
	Rectangle songNameBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.26, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(songNameBox, songName, &songNameBoxSelected);
	strcpy(_map->name, songName);

	// song creator setting
	char creator[50] = {0};
	snprintf(creator, 50, "%s", _map->artist);
	static bool creatorBoxSelected = false;
	Rectangle creatorBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.34, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(creatorBox, creator, &creatorBoxSelected);
	strcpy(_map->artist, creator);

	// preview offset setting
	char musicPreviewOffset[10] = {0};
	musicPreviewOffset[0] = '\0';
	if (_map->musicPreviewOffset != 0)
		snprintf(musicPreviewOffset, 10, "%i", (int)(_map->musicPreviewOffset * 1000));
	static bool musicPreviewOffsetBoxSelected = false;
	Rectangle musicPreviewOffsetBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.42, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(musicPreviewOffsetBox, musicPreviewOffset, &musicPreviewOffsetBoxSelected);
	_map->musicPreviewOffset = atoi(musicPreviewOffset) / 1000.0;
	_map->musicPreviewOffset = fmin(fmax(_map->musicPreviewOffset, 0), _pMusic->size);

	// difficulty setting
	char difficulty[10] = {0};
	difficulty[0] = '\0';
	if (_map->difficulty != 0)
		snprintf(difficulty, 10, "%i", _map->difficulty);
	static bool difficultyBoxSelected = false;
	Rectangle difficultyBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.50, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(difficultyBox, difficulty, &difficultyBoxSelected);
	_map->difficulty = atoi(difficulty);

	// beats per bar setting
	char beats[10] = {0};
	beats[0] = '\0';
	if (_map->beats != 0)
		snprintf(beats, 10, "%i", _map->beats);
	static bool beatsBoxSelected = false;
	Rectangle beatsBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.58, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(beatsBox, beats, &beatsBoxSelected);
	_map->beats = atoi(beats);

	// Drawing text next to the buttons
	char *text = "BPM:";
	float tSize = GetScreenWidth() * 0.025;
	int size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.12, tSize, WHITE);

	text = "Song Offset:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.20, tSize, WHITE);

	text = "Song name:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.28, tSize, WHITE);

	text = "Artist:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.36, tSize, WHITE);

	text = "Preview offset:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() *  0.42, tSize, WHITE);

	text = "Difficulty:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() *  0.50, tSize, WHITE);

	text = "Beats:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() *  0.58, tSize, WHITE);


	// text = "Playback speed:";
	// tSize = GetScreenWidth() * 0.025;
	// size = measureText(text, tSize);
	// drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.50, tSize, WHITE);

	if(IsKeyPressed(KEY_ESCAPE) ||
		interactableButton("back", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.05, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		_pGameplayFunction = &fEditor;
	}

	drawCursor();
}

void fEditorTimingSettings (bool reset)
{
	ClearBackground(BLACK);
	drawBackground();
	dNotes();
	drawVignette();
	drawMusicGraph(0.4);
	drawBars();
	drawProgressBarI(false);

	for(int i = 0; i < _amountTimingSegments && _paTimingSegment; i++)
	{
		DrawCircle(musicTimeToScreen(_paTimingSegment[i].time), GetScreenHeight()*0.3, GetScreenWidth()*0.05, WHITE);
	}

	// Darken background
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	TimingSegment * timSeg = getTimingSignaturePointer(_musicHead);
	// BPM setting
	char bpm[10] = {0};
	if (timSeg->bpm != 0)
		snprintf(bpm, 10, "%i", timSeg->bpm);
	static bool bpmBoxSelected = false;
	Rectangle bpmBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.1, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(bpmBox, bpm, &bpmBoxSelected);
	timSeg->bpm = atoi(bpm);
	timSeg->bpm = fmin(fmax(timSeg->bpm, 0), 300);

	// time setting
	char time[10] = {0};
	if (timSeg->time != 0)
		snprintf(time, 10, "%i", (int)(timSeg->time*1000));
	static bool timeBoxSelected = false;
	Rectangle timeBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.18, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(timeBox, time, &timeBoxSelected);
	timSeg->time = atoi(time) / 1000.0;

	// beats setting
	char beats[10] = {0};
	if (timSeg->beats != 0)
		snprintf(beats, 10, "%i", timSeg->beats);
	static bool beatsBoxSelected = false;
	Rectangle beatsBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.26, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(beatsBox, beats, &beatsBoxSelected);
	timSeg->beats = atoi(beats);
	timSeg->beats = fmin(fmax(timSeg->beats, 0), 300);

	// Drawing text next to the buttons
	char *text = "BPM:";
	float tSize = GetScreenWidth() * 0.025;
	int size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.12, tSize, WHITE);

	text = "Time:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.20, tSize, WHITE);

	text = "Beats:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.28, tSize, WHITE);

	if (interactableButton("back", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.15, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		_pGameplayFunction = &fEditor;
	}

	drawCursor();
}

void fEditorNoteSettings(bool reset)
{
	ClearBackground(BLACK);
	drawBackground();
	dNotes();
	drawVignette();
	drawMusicGraph(0.4);
	drawBars();
	drawProgressBarI(false);
	// Darken background
	DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.5));

	if (interactableButton("Animation", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.5, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		//Run animation tab
		_pGameplayFunction = &fEditorAnimation;
		for(int i = 0; i < _amountSelectedNotes; i++)
			doAction(ComChangeNote, findClosestNote(_papNotes, _amountNotes, _selectedNotes[i]->time), i == 0);
		return;
	}

	if (interactableButton("back", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.15, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		_pGameplayFunction = &fEditor;
		//apply changes to first note to all selected notes
		printf("applying to all selected notes\n");
		Note * firstNote = _selectedNotes[0];

		char path[100];

		if(firstNote->texture_File)
		{
			snprintf(path, 100, "maps/%s/%s", _map->folder, firstNote->texture_File);
			firstNote->custTex = addCustomTexture(path);
		}
		
		if(firstNote->hitSE_File)
		{
			snprintf(path, 100, "maps/%s/%s", _map->folder, firstNote->hitSE_File);
			firstNote->custSound = addCustomSound(firstNote->hitSE_File);
		}

		for(int i = 1; i < _amountSelectedNotes; i++)
		{
			if(firstNote->anim != 0 && firstNote->animSize > 0)
			{
				if(_selectedNotes[i]->anim == 0)
					_selectedNotes[i]->anim = malloc(sizeof(Frame)*firstNote->animSize);
				else
					_selectedNotes[i]->anim = realloc(_selectedNotes[i]->anim, sizeof(Frame)*firstNote->animSize);
				
				_selectedNotes[i]->animSize = firstNote->animSize;

				for(int key = 0; key < firstNote->animSize; key++)
				{
					_selectedNotes[i]->anim[key] = firstNote->anim[key];
				}
			}
			
			if(firstNote->custSound != 0 && firstNote->custSound != _selectedNotes[i]->custSound)
			{
				if(_selectedNotes[i]->custSound != 0)
				{
					removeCustomSound(_selectedNotes[i]->hitSE_File);
					freeArray(_selectedNotes[i]->hitSE_File);
				}
				_selectedNotes[i]->hitSE_File = malloc(strlen(firstNote->hitSE_File)+1);
				strcpy(_selectedNotes[i]->hitSE_File, firstNote->hitSE_File);
				_selectedNotes[i]->custSound = addCustomSound(firstNote->custSound->file);
			}

			if(firstNote->custTex != 0 && firstNote->custTex != _selectedNotes[i]->custTex)
			{
				if(_selectedNotes[i]->custTex != 0)
				{
					removeCustomTexture(_selectedNotes[i]->texture_File);
					freeArray(_selectedNotes[i]->texture_File);
				}
				_selectedNotes[i]->texture_File = malloc(strlen(firstNote->texture_File)+1);
				strcpy(_selectedNotes[i]->texture_File, firstNote->texture_File);
				_selectedNotes[i]->custTex = addCustomTexture(firstNote->custTex->file);
			}

			_selectedNotes[i]->health = firstNote->health;
		}
		return;
	}

	char sprite[100] = {0};
	sprite[0] = '\0';
	if (_selectedNotes[0]->texture_File != 0)
		snprintf(sprite, 100, "%s", _selectedNotes[0]->texture_File);
	
	static bool spriteBoxSelected = false;
	Rectangle spriteBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.1, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(spriteBox, sprite, &spriteBoxSelected);
	if (strlen(sprite) != 0)
	{
		if (_selectedNotes[0]->texture_File == 0)
			_selectedNotes[0]->texture_File = malloc(sizeof(char) * 100);
		strcpy(_selectedNotes[0]->texture_File, sprite);
	}else if(_selectedNotes[0]->texture_File != 0)
	{
		free(_selectedNotes[0]->texture_File);
		_selectedNotes[0]->texture_File = 0;
	}

	char *text = "sprite file:";
	float tSize = GetScreenWidth() * 0.025;
	int size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.1, tSize, WHITE);

	

	// health setting
	char health[10] = {0};
	bool theSame = false;
	for (int i = 0; i < _amountSelectedNotes; i++)
	{
		if (_selectedNotes[0]->health != _selectedNotes[i]->health) {
			theSame = false;
			break;
		}
		else
			theSame = true;
	}
	if (theSame)
	{
		snprintf(health, 10, "%i", (int)(_selectedNotes[0]->health));
	}
	
	static bool healthBoxSelected = false;
	Rectangle healthBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.2, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(healthBox, health, &healthBoxSelected);
	if (healthBoxSelected)
	{
		for (int i = 0; i < _amountSelectedNotes; i++)
		{
			_selectedNotes[i]->health = atoi(health);
			_selectedNotes[i]->health = (int)(fmin(fmax(_selectedNotes[i]->health, 0), 9));
		}
	}

	text = "health:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.2, tSize, WHITE);


	char hitSound[100] = {0};
	hitSound[0] = '\0';
	if (_selectedNotes[0]->hitSE_File != 0)
		snprintf(hitSound, 100, "%s", _selectedNotes[0]->hitSE_File);
	static bool hitSoundBoxSelected = false;
	Rectangle hitSoundBox = (Rectangle){.x = GetScreenWidth() * 0.3, .y = GetScreenHeight() * 0.3, .width = GetScreenWidth() * 0.3, .height = GetScreenHeight() * 0.07};
	textBox(hitSoundBox, hitSound, &hitSoundBoxSelected);
	if (strlen(hitSound) != 0)
	{
		if (_selectedNotes[0]->hitSE_File == 0)
			_selectedNotes[0]->hitSE_File = malloc(sizeof(char) * 100);
		strcpy(_selectedNotes[0]->hitSE_File, hitSound);
	}else if(_selectedNotes[0]->hitSE_File != 0)
	{
		free(_selectedNotes[0]->hitSE_File);
		_selectedNotes[0]->hitSE_File = 0;
	}

	text = "hitSound file:";
	tSize = GetScreenWidth() * 0.025;
	size = measureText(text, tSize);
	drawText(text, GetScreenWidth() * 0.2 - size / 2, GetScreenHeight() * 0.3, tSize, WHITE);

	if (interactableButton("remove Animation", 0.025, GetScreenWidth() * 0.2, GetScreenHeight() * 0.7, GetScreenWidth() * 0.3, GetScreenHeight() * 0.07))
	{
		for(int i = 0; i < _amountSelectedNotes; i++)
		{
			if(_selectedNotes[i]->anim != 0)
				free(_selectedNotes[i]->anim);

			_selectedNotes[i]->anim = 0;
			_selectedNotes[i]->animSize = 0;
		}
	}

	drawCursor();
}

void fEditorAnimation (bool reset)
{
	ClearBackground(BLACK);
	drawVignette();

	//animation :)
	static bool timeLineSelected = false;
	static float timeLine = 0;
	int value = timeLine * 100;
	
	if(_selectedNotes[0]->anim == 0)
	{
		_selectedNotes[0]->anim = malloc(sizeof(Frame) * 2);
		_selectedNotes[0]->anim[0].time = 0;
		_selectedNotes[0]->anim[0].vec = (Vector2){.x=1, .y=0.5};
		_selectedNotes[0]->anim[1].time = 1;
		_selectedNotes[0]->anim[1].vec = (Vector2){.x=0, .y=0.5};
		_selectedNotes[0]->animSize = 2;
	}

	slider((Rectangle){.x=0, .y=GetScreenHeight()*0.9, .width=GetScreenWidth(), .height=GetScreenHeight()*0.03}, &timeLineSelected, &value, 100, -100);
	timeLine = value / 100.0;
	drawNote(timeLine*_scrollSpeed+_selectedNotes[0]->time, _selectedNotes[0], WHITE, 0);
	Frame * anim = _selectedNotes[0]->anim;
	for(int key = 0; key < _selectedNotes[0]->animSize; key++)
	{
		//draw keys
		if(interactableButton("k", 0.01, (anim[key].time)*GetScreenWidth()-GetScreenWidth()*0.01, GetScreenHeight()*0.85, GetScreenWidth()*0.02, GetScreenHeight()*0.05))
		{
			//delete key
			if(_selectedNotes[0]->animSize <= 2)
				break;
			_selectedNotes[0]->animSize --;
			for(int k = key; k < _selectedNotes[0]->animSize; k++)
			{
				anim[k].time = anim[k+1].time;
				anim[k].vec = anim[k+1].vec;
			}
			_selectedNotes[0]->anim = realloc(_selectedNotes[0]->anim, sizeof(Frame)*_selectedNotes[0]->animSize);
		}
	}

	if (interactableButton("back", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.5, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		//Run animation tab
		_pGameplayFunction = &fEditorNoteSettings;
		return;
	}

	if(IsMouseButtonReleased(0) && GetMouseY() < GetScreenHeight()*0.85)
	{
		int index = -1;
		for(int key = 0; key < _selectedNotes[0]->animSize; key++)
		{
			if((timeLine+1)/2 == _selectedNotes[0]->anim[key].time)
			{
				index = key;
				break;
			}
		}
		if(index != -1)
		{
			//modify existing frame
			anim[index].vec = (Vector2){.x=GetMouseX()/(float)GetScreenWidth(),.y=GetMouseY()/(float)GetScreenHeight()+0.05};
		}else
		{
			//create new frame
			_selectedNotes[0]->animSize++;
			_selectedNotes[0]->anim = realloc(_selectedNotes[0]->anim, _selectedNotes[0]->animSize*sizeof(Frame));
			int newIndex = 0;
			for(int key = _selectedNotes[0]->animSize-2; key > 0; key--)
			{
				if(_selectedNotes[0]->anim[key].time > (timeLine+1)/2)
				{
					_selectedNotes[0]->anim[key+1] = _selectedNotes[0]->anim[key];
					newIndex = key;
				}
			}
			_selectedNotes[0]->anim[newIndex].time = (timeLine+1)/2;
			_selectedNotes[0]->anim[newIndex].vec = (Vector2){.x=GetMouseX()/(float)GetScreenWidth(),.y=GetMouseY()/(float)GetScreenHeight()+0.05};
		}
	}

	drawCursor();
}

void fEditor(bool reset)
{
	if(reset)
	{
		loadMap();
		for(int i = 0; i < COMMANDBUFFER; i++)
		{
			freeCommand(i);
			_CommandIndex = 0;
		}
		startMusic();
		_musicPlaying = false;
		return;
	}


	ClearBackground(BLACK);
	drawBackground();
	dNotes();
	drawVignette();
	drawMusicGraph(0.4);
	drawBars();
	if(drawProgressBarI(true))
		_musicPlaying = false;

	for(int i = 0; i < _amountTimingSegments && _paTimingSegment; i++)
	{
		DrawCircle(musicTimeToScreen(_paTimingSegment[i].time), GetScreenHeight()*0.3, GetScreenWidth()*0.05, WHITE);
	}

	TimingSegment timeSeg = getTimingSignature(_musicHead);
	float secondsPerBeat = (60.0/timeSeg.bpm) / _barMeasureCount;
	if (_musicPlaying)
	{
		_musicHead += GetFrameTime() * _musicSpeed;
		if (_amountNotes > 0 && _noteIndex < _amountNotes && getMusicHead() > _papNotes[_noteIndex]->time)
		{
			if(_papNotes[_noteIndex]->custSound)
				playAudioEffect(_papNotes[_noteIndex]->custSound->sound);
			else
				playAudioEffect(_hitSe);

			_noteIndex++;
			while(_noteIndex < _amountNotes -1 && getMusicHead() > _papNotes[_noteIndex]->time)
			{
				_noteIndex++;
			}
		}

		while(_noteIndex > 0 && getMusicHead() < _papNotes[_noteIndex-1]->time)
			_noteIndex--;
		
		if (endOfMusic())
		{
			_musicPlaying = false;
			_musicPlaying = false;
		}
	}else
	{
		setMusicFrameCount();
		_noteIndex = 0;
		static float timeRightKey = 0;
		//Snapping left and right with arrow keys
		if (IsKeyDown(KEY_RIGHT))
		{
			timeRightKey += GetFrameTime()*10;

			if(IsKeyPressed(KEY_RIGHT) || (((int)timeRightKey)%2 == 1 && timeRightKey > 7))
			{
				timeRightKey += 1;
				double before = _musicHead;
				// Snap to closest beat
				_musicHead = roundf((getMusicHead() - timeSeg.time) / secondsPerBeat) * secondsPerBeat;
				// Add the offset
				_musicHead += timeSeg.time;
				// Add the bps to the music head
				if(before >= _musicHead-0.001) _musicHead += secondsPerBeat;
				if(before >= _musicHead-0.001) _musicHead += secondsPerBeat;
				// // snap it again (it's close enough right?????)
				// _musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
				_musicPlaying = false;
			}
		}else
			timeRightKey = 0;

		static float timeLeftKey = 0;
		if (IsKeyDown(KEY_LEFT))
		{
			timeLeftKey += GetFrameTime()*10;

			if(IsKeyPressed(KEY_LEFT) || (((int)timeLeftKey)%2 == 1 && timeLeftKey > 7))
			{
				secondsPerBeat = (60.0/getTimingSignature(_musicHead-0.001).bpm) / _barMeasureCount;
				double before = _musicHead;
				_musicHead = floorf((getMusicHead() - timeSeg.time) / secondsPerBeat) * secondsPerBeat;
				_musicHead += timeSeg.time;	
				if(before <= _musicHead+0.001) _musicHead -= secondsPerBeat;
				if(before <= _musicHead+0.001) _musicHead -= secondsPerBeat;
				// _musicHead = roundf(getMusicHead() / secondsPerBeat) * secondsPerBeat;
				_musicPlaying = false;
			}
		}else
			timeLeftKey = 0;

		//Scroll timeline with mousewheel
		if(!IsKeyDown(KEY_LEFT_CONTROL))
		{
			if(GetMouseWheelMove() != 0)
				_musicHead = roundf((getMusicHead() - timeSeg.time) / secondsPerBeat) * secondsPerBeat +timeSeg.time;
			if (GetMouseWheelMove() < 0)
				_musicHead += secondsPerBeat;
			if (GetMouseWheelMove() > 0)
				_musicHead -= secondsPerBeat;
		}
		if (IsMouseButtonDown(2))
		{
			_musicHead -= GetMouseDelta().x / GetScreenWidth() * _scrollSpeed;
		}
		//Pause menu
		if (IsKeyPressed(KEY_ESCAPE))
		{
			_pGameplayFunction = &fPause;
			_pNextGameplayFunction = &fEditor;
			free(_selectedNotes);
			_selectedNotes = 0;
			_amountSelectedNotes = 0;
			return;
		}
		//Small optimisation defined in main.c
		if (_isKeyPressed)
		{
			float closestTime = 55;
			int closestIndex = 0;
			for (int i = 0; i < _amountNotes; i++)
			{
				if (closestTime > fabs(_papNotes[i]->time - getMusicHead()))
				{
					closestTime = fabs(_papNotes[i]->time - getMusicHead());
					closestIndex = i;
				}
			}
			if (IsKeyPressed(KEY_Z) && IsKeyDown(KEY_LEFT_CONTROL))
			{
				undo();
			}else if (IsKeyPressed(KEY_Z) && !IsKeyDown(KEY_LEFT_CONTROL) && closestTime > 0.003f)
			{
				doAction(ComAdd, newNote(getMusicHead()), 1);
				_noteIndex = closestIndex;
			}

			if (IsKeyPressed(KEY_A))
			{
				float pos = roundf((getMusicHead() - timeSeg.time) / secondsPerBeat) * secondsPerBeat + timeSeg.time;
				doAction(ComAdd, newNote(pos), 1);
				_noteIndex = closestIndex;
			}

			if (IsKeyPressed(KEY_D))
			{
				addTimingSignature(_musicHead, _map->bpm);
			}

			if (IsKeyPressed(KEY_F))
			{
				removeTimingSignature(_musicHead);
			}


			bool delKey = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_DELETE);
			if (delKey && closestTime < _maxMargin && !_amountSelectedNotes)
			{
				doAction(ComRemove, closestIndex, 1);

				removeNote(closestIndex);
				if(closestIndex >= _noteIndex)
					_noteIndex--;
			}else if(delKey)
			{
				int cost = 1;
				while(_amountSelectedNotes > 0)
				{
					int index = findClosestNote(_papNotes, _amountNotes, _selectedNotes[0]->time);
					doAction(ComRemove, index, cost);
					cost = 0;
					removeNote(index);
				}
				freeArray(_selectedNotes);
				_amountSelectedNotes = 0;
			}

			if (IsKeyPressed(KEY_C) && !_musicPlaying)
			{
				_musicHead = roundf((getMusicHead() - timeSeg.time) / secondsPerBeat) * secondsPerBeat + timeSeg.time;
			}

			if (IsKeyPressed(KEY_V) && IsKeyDown(KEY_LEFT_CONTROL) && closestTime > 0.03f && _amountSelectedNotes > 0)
			{
				//copy currently selected notes at _musichead position

				//get begin point of notes
				float firstNote = -1;
				for(int i = 0; i < _amountSelectedNotes; i++)
				{
					if(firstNote == -1)
						firstNote = _selectedNotes[i]->time;
					else if(_selectedNotes[i]->time < firstNote)
					{
						firstNote = _selectedNotes[i]->time;
					}
				}
				//copy over notes
				int cost = 1;
				for(int i = 0; i < _amountSelectedNotes; i++)
				{
					int note = newNote(_musicHead+_selectedNotes[i]->time-firstNote);
					doAction(ComAdd, note, cost);
					cost = 0;
					if(_selectedNotes[i]->anim)
					{
						_papNotes[note]->anim = malloc(sizeof(Frame)*_selectedNotes[i]->animSize);
						for(int j = 0; j < _selectedNotes[i]->animSize; j++)
						{
							_papNotes[note]->anim[j] = _selectedNotes[i]->anim[j];
						}
						_papNotes[note]->animSize = _selectedNotes[i]->animSize;
					}

					if(_selectedNotes[i]->hitSE_File)
					{
						_papNotes[note]->hitSE_File = malloc(100);
						strcpy(_papNotes[note]->hitSE_File, _selectedNotes[i]->hitSE_File);
						char tmpStr[100];
						snprintf(tmpStr, 100, "maps/%s/%s", _map->folder, _selectedNotes[i]->hitSE_File);
						_papNotes[note]->custSound = addCustomSound(tmpStr);
					}

					if(_selectedNotes[i]->texture_File)
					{
						_papNotes[note]->texture_File = malloc(100);
						strcpy(_papNotes[note]->texture_File, _selectedNotes[i]->texture_File);
						char tmpStr[100];
						snprintf(tmpStr, 100, "maps/%s/%s", _map->folder, _selectedNotes[i]->texture_File);
						_papNotes[note]->custTex = addCustomTexture(tmpStr);
					}
				}
			}

			if (IsKeyPressed(KEY_E) && _barMeasureCount <= 32)
			{
				_barMeasureCount += 1;
			}

			if (IsKeyPressed(KEY_Q) && _barMeasureCount >= 2)
			{
				_barMeasureCount -= 1;
			}
		}
	}

	//Change scrollspeed
	if (IsKeyPressed(KEY_UP) || (GetMouseWheelMove() > 0 && IsKeyDown(KEY_LEFT_CONTROL)))
		_scrollSpeed *= 1.2;
	if (IsKeyPressed(KEY_DOWN) || (GetMouseWheelMove() < 0 && IsKeyDown(KEY_LEFT_CONTROL)))
		_scrollSpeed /= 1.2;
	if (_scrollSpeed == 0)
		_scrollSpeed = 0.01;
	
	//Selecting notes
	if (IsMouseButtonPressed(0) && GetMouseY() > GetScreenHeight() * 0.3 && GetMouseY() < GetScreenHeight() * 0.6)
	{
		if(!IsKeyDown(KEY_LEFT_SHIFT))
		{
			int note = findClosestNote(_papNotes, _amountNotes, screenToMusicTime(GetMouseX()));
			bool unselect = false;
			if(_amountSelectedNotes == 1 && _selectedNotes[0] == _papNotes[note])
				unselect = true;
			free(_selectedNotes);
			_amountSelectedNotes = 0;
			_selectedNotes = 0;
			if(!unselect)
				addSelectNote(findClosestNote(_papNotes, _amountNotes, screenToMusicTime(GetMouseX())));
		}
		else
			addSelectNote(findClosestNote(_papNotes, _amountNotes, screenToMusicTime(GetMouseX())));
	}
	//prevent bugs by setting musicHead to 0 when it gets below 0
	if (getMusicHead() < 0)
		_musicHead = 0;
	//prevent bugs by setting muiscHead to the max song duration
	if (getMusicHead() > getMusicDuration())
		_musicHead = getMusicDuration();

	if (IsKeyPressed(KEY_SPACE))
	{
		_musicPlaying = !_musicPlaying;
		if (!_musicPlaying && floorf(getMusicHead() / secondsPerBeat) * secondsPerBeat < getMusicDuration())
		{
			_musicHead = floorf((getMusicHead()-timeSeg.time) / secondsPerBeat) * secondsPerBeat;
			_musicHead += timeSeg.time;
		}
		_noteIndex = findClosestNote(_papNotes, _amountNotes, _musicHead);
		
	}
	
	if(_musicHead < 0)
		_musicHead = 0;
	if(_musicHead > getMusicDuration())
		_musicHead = getMusicDuration();


	if (_amountSelectedNotes > 0)
	{
		if (interactableButton("Note settings", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.25, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
		{
			_pGameplayFunction = &fEditorNoteSettings;
		}
	}

	if (interactableButton("Song settings", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.05, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
	{
		_pGameplayFunction = &fEditorSongSettings;
	}

	for(int i = 0; i < _amountSelectedNotes; i++)
	{
		DrawCircle(musicTimeToScreen(_selectedNotes[i]->time), GetScreenHeight()*0.55, GetScreenWidth()*0.013, BLACK);
		DrawCircle(musicTimeToScreen(_selectedNotes[i]->time), GetScreenHeight()*0.55, GetScreenWidth()*0.01, WHITE);
	}
	
	if(getTimingSignaturePointer(_musicHead))
	{
		if (interactableButton("Timing settings", 0.025, GetScreenWidth() * 0.8, GetScreenHeight() * 0.15, GetScreenWidth() * 0.2, GetScreenHeight() * 0.07))
		{
			_pGameplayFunction = &fEditorTimingSettings;
		}
	}

	// Speed slider
	static bool speedSlider = false;
	int speed = _musicSpeed * 4;
	slider((Rectangle){.x = GetScreenWidth() * 0.1, .y = GetScreenHeight() * 0.05, .width = GetScreenWidth() * 0.2, .height = GetScreenHeight() * 0.03}, &speedSlider, &speed, 8, 1);
	_musicSpeed = speed / 4.0;
	if (interactableButton("reset", 0.03, GetScreenWidth() * 0.32, GetScreenHeight() * 0.05, GetScreenWidth() * 0.1, GetScreenHeight() * 0.05))
		_musicSpeed = 1;

	drawCursor();
}