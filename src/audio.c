#include "audio.h"
#define MINIAUDIO_IMPLEMENTATION
#include "include/miniaudio.h"

#include "files.h"


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int _amountNotes ,_musicLength, _musicFrameCount;
extern float _musicHead, _scrollSpeed;
extern Map * _map;
extern Settings _settings;

#define EFFECT_BUFFER_SIZE 48000 * 4 * 4

ma_decoder _decoder;
ma_device_config _deviceConfig;
ma_device _device;

void *_pHitSE;
int _hitSE_Size;

void *_pMissHitSE;
int _missHitSE_Size;

void *_pMissSE;
int _missSE_Size;

void *_pButtonSE;
int _buttonSE_Size;

void *_pClickPress;
int _clickPressSE_Size;

void *_pClickRelease;
int _clickReleaseSE_Size;

void *_pFailSE;
int _failSE_Size;

void *_pFinishSE;
int _finishSE_Size;

void *_pEffectsBuffer;
int _effectOffset;


//Where is the current audio
float _musicHead = 0;
bool _musicPlaying;
bool _musicLoops = true;

int _musicFrameCount = 0;
int _musicLength = 0;

void *_pMusic = 0;

 

int getSamplePosition(float time) {
	return time*_decoder.outputSampleRate*2;
}
//Get duration of music in seconds
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
	return _map->bpm * getMusicDuration() / 60 / 4;
}

int getBeatsCount()
{
	return _map->bpm * getMusicDuration() / 60;
}

void setMusicStart()
{
	_musicFrameCount = 0;
}

void randomMusicPoint()
{
	_musicFrameCount = rand();
}

void * loadAudio(char * file, ma_decoder * decoder, int * audioLength)
{
	ma_result result;
	printf("loading sound effect %s\n", file);
	ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 2, 48000);
	decoder_config.resampling.linear.lpfOrder = MA_MAX_FILTER_ORDER;
	result = ma_decoder_init_file(file, &decoder_config, decoder);
    if (result != MA_SUCCESS) {
        printf("failed to open music file %s\n", file);
		exit(0);
    }
	int lastFrame = -1;
	ma_decoder_get_length_in_pcm_frames(decoder, (long long unsigned int *)audioLength);
	void * pAudio = calloc(sizeof(float)*2*2, *audioLength); //added some patting to get around memory issue //todo fix this work around
	void * pCursor = pAudio;
	while(decoder->readPointerInPCMFrames !=lastFrame)
	{
		lastFrame = decoder->readPointerInPCMFrames;
		ma_decoder_read_pcm_frames(decoder, pCursor, 256, NULL);
		pCursor += sizeof(float)*2*256;
	}
	ma_decoder_uninit(decoder);
	return pAudio;
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	float globalVolume = _settings.volumeGlobal/100.0;
	float musicVolume = _settings.volumeMusic/100.0 * globalVolume;
	float audioEffectVolume = _settings.volumeSoundEffects/100.0 * globalVolume;
	// music
	if (_musicPlaying)
	{
		if (_musicLength > _musicFrameCount && _musicFrameCount > 0)
		{
			for (int i = 0; i < frameCount * 2; i++)
			{
				((float *)pOutput)[i] = ((float *)_pMusic)[i + _musicFrameCount*2]*musicVolume;
			}
			// memcpy(pOutput, _pMusic + (_musicFrameCount) * sizeof(float) * 2, frameCount * sizeof(float) * 2);
		}
		else if(_musicLoops)
			_musicFrameCount = _musicFrameCount%_musicLength;
		_musicFrameCount += frameCount;
	}
	// sound effects

	for (int i = 0; i < frameCount * 2; i++)
	{
		((float *)pOutput)[i] += ((float *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)]*audioEffectVolume;
		((float *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] = 0;
	}
	_effectOffset += frameCount * 2;

	(void)pInput;
}

void audioInit()
{
		//todo do this smarter
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); //4 second long buffer
	ma_decoder tmp;
	_pHitSE = loadAudio("hit.mp3", &tmp, &_hitSE_Size);
	_pMissHitSE = loadAudio("missHit.mp3", &tmp, &_missHitSE_Size);
	_pMissSE = loadAudio("missHit.mp3", &tmp, &_missSE_Size);
	_pButtonSE = loadAudio("button.mp3", &tmp, &_buttonSE_Size);

	_pClickPress = loadAudio("clickPress.mp3", &tmp, &_clickPressSE_Size);
	_pClickRelease = loadAudio("clickRelease.mp3", &tmp, &_clickReleaseSE_Size);

	_pFinishSE = loadAudio("hit.mp3", &tmp, &_finishSE_Size);
	_pFailSE = loadAudio("missHit.mp3", &tmp, &_failSE_Size);


	loadMusic("menuMusic.mp3");


	_musicPlaying = true;
	_deviceConfig = ma_device_config_init(ma_device_type_playback);
    _deviceConfig.playback.format   = ma_format_f32;
    _deviceConfig.playback.channels = 2;
    _deviceConfig.sampleRate        = 48000;
    _deviceConfig.dataCallback      = data_callback;
	// _deviceConfig.periodSizeInMilliseconds = 300;
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
}


void loadMusic(char * file)
{
	_musicPlaying = false;
	if(_pMusic != 0)
	{
		//unload previous music
		free(_pMusic);
	}

	_pMusic = loadAudio(file, &_decoder, &_musicLength);
}

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
		((float *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] += ((float *)effect)[i];
	}
}

void startMusic()
{
	_musicPlaying = true;
	_musicFrameCount = 1;
}

void stopMusic()
{
	_musicFrameCount = 0;
	_musicHead = 0;
	_musicPlaying = false;
}

void setMusicFrameCount()
{
    _musicFrameCount = _musicHead*_decoder.outputSampleRate;
}