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
	if(bpm < 1)
		bpm = 100;
	
	if(!_paTimingSegment)
	{
		_paTimingSegment = malloc(sizeof(TimingSegment));
		_amountTimingSegments = 1;
		_paTimingSegment[0].time = time;
		_paTimingSegment[0].bpm = bpm;
		_paTimingSegment[0].beats = 4;
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

void loadCustomAssets(Note * note)
{
	if(note->hitSE_File)
	{
		char fullPath [200];
		snprintf(fullPath, 200, "%s/%s", _map->folder, note->hitSE_File);
		note->custSound = addCustomSound(fullPath);
	}

	if(note->texture_File)
	{
		char fullPath [200];
		snprintf(fullPath, 200, "%s/%s", _map->folder, note->texture_File);
		note->custTex = addCustomTexture(fullPath);
	}
}

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
			loadCustomAssets(_papNotes[index]);
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
	dNotes();
	drawVignette();
	drawMusicGraph(0.4);
	drawBars();
	drawProgressBarI(false);


	drawCSS("theme/editor/songSettings.css");

	// BPM setting
	_map->bpm = UIValueInteractable(_map->bpm, "BPM_Box");
	_map->offset = UIValueInteractable(_map->offset, "offsetBox");

	UITextBox(_map->name, "nameBox");
	UITextBox(_map->artist, "artistBox");
	UITextBox(_map->mapCreator, "mapCreatorBox");
	
	_map->musicPreviewOffset = UIValueInteractable(_map->musicPreviewOffset, "previewOffsetBox");
	_map->difficulty = UIValueInteractable(_map->difficulty, "difficultyBox");
	_map->beats = UIValueInteractable(_map->beats, "beatsBox");

	if(IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	{
		_pGameplayFunction = &fEditor;
	}

	drawCursor();
}

void fEditorTimingSettings (bool reset)
{
	static TimingSegment * timSeg = 0;
	if(reset)
	{
		timSeg = getTimingSignaturePointer(_musicHead);
		return;
	}

	ClearBackground(BLACK);
	drawBackground();
	dNotes();
	drawVignette();
	drawMusicGraph(0.4);
	drawBars();
	drawProgressBarI(false);

	for(int i = 0; i < _amountTimingSegments && _paTimingSegment; i++)
	{
		DrawCircle(musicTimeToScreen(_paTimingSegment[i].time), getHeight()*0.3, getWidth()*0.05, WHITE);
	}

	drawCSS("theme/editor/timingSettings.css");

	// BPM setting
	timSeg->bpm = UIValueInteractable(timSeg->bpm, "BPM_Box");
	timSeg->time = UIValueInteractable(timSeg->time*1000, "timeBox") / 1000.0;
	timSeg->beats = UIValueInteractable(timSeg->beats, "beatsBox");

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
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

	drawCSS("theme/editor/noteSettings.css");

	if (UIBUttonPressed("animationButton"))
	{
		//Run animation tab
		_pGameplayFunction = &fEditorAnimation;
		return;
	}

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	{
		_pGameplayFunction = &fEditor;
		//apply changes to first note to all selected notes
		printf("applying to all selected notes\n");
		Note * firstNote = _selectedNotes[0];

		char path[100];

		if(firstNote->texture_File)
		{
			if(!strlen(firstNote->texture_File))
			{
				freeArray(firstNote->texture_File);
			}else
			{
				snprintf(path, 100, "%s/%s", _map->folder, firstNote->texture_File);
				firstNote->custTex = addCustomTexture(path);
				if(!firstNote->custTex)
				{
					freeArray(firstNote->texture_File);
				}
			}
		}
		
		if(firstNote->hitSE_File)
		{
			if(!strlen(firstNote->hitSE_File))
			{
				freeArray(firstNote->hitSE_File);
			}else
			{
				snprintf(path, 100, "%s/%s", _map->folder, firstNote->hitSE_File);
				firstNote->custSound = addCustomSound(firstNote->hitSE_File);
				if(!firstNote->custSound)
				{
					freeArray(firstNote->hitSE_File);
				}
			}
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


	_selectedNotes[0]->health = UIValueInteractable(_selectedNotes[0]->health, "healthBox");

	char sprite[100] = {0};
	sprite[0] = '\0';
	if (_selectedNotes[0]->texture_File != 0)
		snprintf(sprite, 100, "%s", _selectedNotes[0]->texture_File);
	
	UITextBox(sprite, "textureFileBox");

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


	char hitSound[100] = {0};
	hitSound[0] = '\0';
	if (_selectedNotes[0]->hitSE_File != 0)
		snprintf(hitSound, 100, "%s", _selectedNotes[0]->hitSE_File);

	
	UITextBox(hitSound, "hitsoundBox");
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

	if (UIBUttonPressed("removeAnimationButton"))
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

	float position = musicTimeToScreen(_musicHead);
	DrawRectangle(0, 0, position, getHeight(), ColorAlpha(WHITE, 0.15));

	float middle = getWidth()*0.5;
	float timingHelper = getWidth()*0.005;
	DrawRectangle(middle-timingHelper, getHeight()*0.85, timingHelper*2, getHeight(), ColorAlpha(WHITE, 0.3));

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

	slider((Rectangle){.x=0, .y=getHeight()*0.9, .width=getWidth(), .height=getHeight()*0.03}, &timeLineSelected, &value, 100, -100);
	timeLine = value / 100.0;
	drawNote(timeLine*_scrollSpeed+_selectedNotes[0]->time, _selectedNotes[0], WHITE, 0);
	Frame * anim = _selectedNotes[0]->anim;
	for(int key = 0; key < _selectedNotes[0]->animSize; key++)
	{
		//draw keys
		if(interactableButton("k", 0.01, (anim[key].time)*getWidth()-getWidth()*0.01, getHeight()*0.85, getWidth()*0.02, getHeight()*0.05))
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

	if (IsKeyPressed(KEY_ESCAPE) || interactableButton("back", 0.025, getWidth() * 0.8, getHeight() * 0.5, getWidth() * 0.2, getHeight() * 0.07))
	{
		//Run animation tab
		_pGameplayFunction = &fEditorNoteSettings;
		return;
	}

	if(IsMouseButtonReleased(0) && GetMouseY() < getHeight()*0.85)
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
			anim[index].vec = (Vector2){.x=GetMouseX()/(float)getWidth(),.y=GetMouseY()/(float)getHeight()+0.05};
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
			_selectedNotes[0]->anim[newIndex].vec = (Vector2){.x=GetMouseX()/(float)getWidth(),.y=GetMouseY()/(float)getHeight()+0.05};
		}
	}

	drawCursor();
}

void fEditor(bool reset)
{

	if(reset)
	{
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
	

	for(int i = 0; i < _amountTimingSegments && _paTimingSegment; i++)
	{
		int x = musicTimeToScreen(_paTimingSegment[i].time);
		int y = getHeight()*0.3;
		int width = getHeight()*0.05;

		DrawCircle(x, y, width, ColorAlpha(BLACK, 0.6));

		width *= 1.3;

		x -= width / 2;
		y -= width / 2;

		drawTextureCorrectAspectRatio(_iconTime, WHITE,
			(Rectangle){.x=x,.y=y, .width=width, .height=width}, 0);
	}

	drawCSS("theme/editor/editor.css");

	if(drawProgressBarI(true))
		_musicPlaying = false;

	CSS_Object * playButton = getCSS_ObjectPointer("playButton");
	if(playButton)
	{
		if(playButton->selected && playButton->active)
			_musicPlaying = true;
		
		playButton->active = !_musicPlaying;
	}

	CSS_Object * pauseButton = getCSS_ObjectPointer("pauseButton");
	if(pauseButton)
	{
		if(pauseButton->selected && pauseButton->active)
			_musicPlaying = false;
		
		pauseButton->active = _musicPlaying;
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
			_musicHead -= GetMouseDelta().x / getWidth() * _scrollSpeed;
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
		}
		
		if (closestTime > 0.003f && ((IsKeyPressed(KEY_Z) && !IsKeyDown(KEY_LEFT_CONTROL)) || UIBUttonPressed("addNoteButton")))
		{
			doAction(ComAdd, newNote(getMusicHead()), 1);
			_noteIndex = closestIndex;
		}

		if (IsKeyPressed(KEY_A) && !IsKeyDown(KEY_LEFT_CONTROL))
		{
			float pos = roundf((getMusicHead() - timeSeg.time) / secondsPerBeat) * secondsPerBeat + timeSeg.time;
			doAction(ComAdd, newNote(pos), 1);
			_noteIndex = closestIndex;
		}

		if (IsKeyPressed(KEY_A) && IsKeyDown(KEY_LEFT_CONTROL))
		{
			for(int i = 0; i < _amountNotes; i++)
				addSelectNote(i);
		}

		if (IsKeyPressed(KEY_D) || UIBUttonPressed("addTimeSigButton"))
		{
			addTimingSignature(_musicHead, _map->bpm);
		}

		if (IsKeyPressed(KEY_F) || UIBUttonPressed("rmTimeSigButton"))
		{
			removeTimingSignature(_musicHead);
		}


		bool delKey = IsKeyPressed(KEY_X) || IsKeyPressed(KEY_DELETE) || UIBUttonPressed("rmNoteButton");
		
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
					snprintf(tmpStr, 100, "%s/%s", _map->folder, _selectedNotes[i]->hitSE_File);
					_papNotes[note]->custSound = addCustomSound(tmpStr);
				}

				if(_selectedNotes[i]->texture_File)
				{
					_papNotes[note]->texture_File = malloc(100);
					strcpy(_papNotes[note]->texture_File, _selectedNotes[i]->texture_File);
					char tmpStr[100];
					snprintf(tmpStr, 100, "%s/%s", _map->folder, _selectedNotes[i]->texture_File);
					_papNotes[note]->custTex = addCustomTexture(tmpStr);
				}
			}
		}

		if (_barMeasureCount <= 32 && (IsKeyPressed(KEY_E) || UIBUttonPressed("barPlusButton")))
		{
			_barMeasureCount *= 2;
		}

		if (_barMeasureCount >= 2 && (IsKeyPressed(KEY_E) || UIBUttonPressed("barMinusButton")))
		{
			_barMeasureCount /= 2;
		}
	}

	_scrollSpeed = UIValueInteractable(_scrollSpeed*10, "zoomSlider") / 10.0;

	if(UIBUttonPressed("zoomResetButton"))
		_scrollSpeed = 4.2 / _settings.zoom;

	//Change scrollspeed
	if (IsKeyPressed(KEY_DOWN) || (GetMouseWheelMove() < 0 && IsKeyDown(KEY_LEFT_CONTROL)))
		_scrollSpeed /= 1.2;

	if (IsKeyPressed(KEY_UP) || (GetMouseWheelMove() > 0 && IsKeyDown(KEY_LEFT_CONTROL)))
		_scrollSpeed *= 1.2;

	
	if (_scrollSpeed == 0)
		_scrollSpeed = 0.01;
	
	//Selecting notes
	if (IsMouseButtonPressed(0) && GetMouseY() > getHeight() * 0.3 && GetMouseY() < getHeight() * 0.6)
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

	CSS_Object * noteSettingsObj = getCSS_ObjectPointer("noteSettingsButton");
	if(noteSettingsObj)
		noteSettingsObj->active = (_amountSelectedNotes > 0);
	
	if (UIBUttonPressed("noteSettingsButton"))
	{
		_musicPlaying = false;
		_pGameplayFunction = &fEditorNoteSettings;
		for(int i = 0; i < _amountSelectedNotes; i++)
			doAction(ComChangeNote, findClosestNote(_papNotes, _amountNotes, _selectedNotes[i]->time), i == 0);
	}

	if (UIBUttonPressed("songSettingsButton"))
	{
		_musicPlaying = false;
		_pGameplayFunction = &fEditorSongSettings;
	}

	for(int i = 0; i < _amountSelectedNotes; i++)
	{
		DrawCircle(musicTimeToScreen(_selectedNotes[i]->time), getHeight()*0.55, getWidth()*0.013, BLACK);
		DrawCircle(musicTimeToScreen(_selectedNotes[i]->time), getHeight()*0.55, getWidth()*0.01, WHITE);
	}
	
	CSS_Object * timingSettingsObj = getCSS_ObjectPointer("timingSettingsButton");
	if(timingSettingsObj)
		timingSettingsObj->active = (getTimingSignaturePointer(_musicHead) != 0);
	
	if (UIBUttonPressed("timingSettingsButton"))
	{
		_musicPlaying = false;
		_pGameplayFunction = &fEditorTimingSettings;
		fEditorTimingSettings(true);
	}

	// Speed slider
	_musicSpeed = (UIValueInteractable(_musicSpeed * 4 - 4, "speedSlider") + 4) / 4.0;

	if (UIBUttonPressed("speedResetButton"))
		_musicSpeed = 1;

	drawCursor();
}