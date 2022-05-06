#ifndef MAINC
#include "shared.h"
#endif

float noLessThanZero(float var)
 {
 	if (var < 0)
 		return 0;
 	return var;
 }

void printAllNotes()
{
	for (int i = 0; i < _amountNotes; i++)
	{
		printf("note %.2f\n", _pNotes[i]);
	}
}

int getSamplePosition(float time) {
	return time*_decoder.outputSampleRate;
}

float getMusicDuration()
{
	return _musicLength / (float)_decoder.outputSampleRate;
}

float getMusicPosition()
{
	return _musicFrameCount / (float)_decoder.outputSampleRate;
}

void fixMusicTime()
{
	if (fabs(_musicHead - getMusicPosition()) > 0.1)
		_musicHead = getMusicPosition();
}

int getBarsCount()
{
	return bpm * getMusicDuration() / 60 / 4;
}

int getBeatsCount()
{
	return bpm * getMusicDuration() / 60;
}

//TODO think of a better name because f used in front of a function is for gameplay functions
float fDistance(float x1, float y1, float x2, float y2)
{
	float x = x1 - x2;
	float y = y1 - y2;
	return sqrtf(fabs(x * x + y * y));
}

float musicTimeToScreen(float musicTime)
{
	float middle = GetScreenWidth() / 2;
	return middle + middle * (musicTime - _musicHead) * (1 / _scrollSpeed);
}

float screenToMusicTime(float x)
{
	float middle = GetScreenWidth() / 2;
	return (x - middle) / (middle * (1 / _scrollSpeed)) + _musicHead;
}