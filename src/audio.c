#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "windowsDefs.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../deps/miniaudio/miniaudio.h"


#define EXTERN_GAMEPLAY
#define EXTERN_MAIN

#include "audio.h"
#include "thread.h"
#include "main.h"

// #include "gameplay/gameplay.h"



#define EFFECT_BUFFER_SIZE 48000 * 15 * sizeof(float)

ma_device_config _deviceConfig;
ma_device _device;

Audio _hitSe;
Audio _missHitSe;
Audio _missSe;
Audio _buttonSe;
Audio _clickPressSe;
Audio _clickReleaseSe;
Audio _failSe;
Audio _finishSe;
Audio _menuMusic;

Audio *_pMusic = 0;


float *_pEffectsBuffer;
int _effectOffset;

double _musicHead = 0, _musicSpeed = 1;
bool _musicPlaying = false, _musicLoops = true, _playMenuMusic = true;
float _musicPreviewTimer = 0;

int _musicFrameCount = 0;
float _musicPreviewOffset;

float _musicVolume, _audioEffectVolume;
int _framesOffset = 0;


int _menuMusicFrameCount = 0;

int getSamplePosition(float time)
{
	return time * 48000 * 2;
}
// Get duration of music in seconds
float getMusicDuration()
{
	return _pMusic->size / (float)48000;
}

float getMusicPosition()
{
	return _musicFrameCount / (float)48000;
}

void fixMusicTime()
{
	_musicHead = getMusicPosition();
}

void setMusicStart()
{
	_musicFrameCount = 0;
}

void randomMusicPoint()
{
	_musicFrameCount = rand();
}

struct decodeAudioArgs
{
	void **buffer;
	char *file;
	int *audioLength;
};

#ifdef __unix
void *decodeAudio(struct decodeAudioArgs *args)
#else
DWORD WINAPI *decodeAudio(struct decodeAudioArgs *args)
#endif
{
	lockLoadingMutex();
	_loading++;
	unlockLoadingMutex();
	ma_result result;
	printf("loading sound effect %s\n", args->file);

	ma_decoder_config decoder_config = ma_decoder_config_init(ma_format_f32, 2, 48000);
	decoder_config.resampling.linear.lpfOrder = 0;
	// decoder_config.resampling.channels = 2;
	ma_decoder decoder = {0};
	long long unsigned int audioLength = 0;
	result = ma_decoder_init_file(args->file, &decoder_config, &decoder);
	if (result != MA_SUCCESS)
	{
		printf("failed to open music file %s %i\n", args->file, result);
		*args->buffer = 0;
		*args->audioLength = 0;
		free(args->file);
		lockLoadingMutex();
		_loading--;
		unlockLoadingMutex();
		return NULL;
		// exit(0);
	}
	ma_decoder_get_length_in_pcm_frames(&decoder, &audioLength);
	if(*args->audioLength && *args->buffer)
		free(*args->buffer);
	*args->buffer = calloc(sizeof(float) * 2 * 2, audioLength);
	void *pCursor = *args->buffer;
	int size = 0;
	int lastFrame = -1;


	while (decoder.readPointerInPCMFrames != lastFrame)
	{
		lastFrame = decoder.readPointerInPCMFrames;
		ma_decoder_read_pcm_frames(&decoder, pCursor, 4096, NULL);
		pCursor += sizeof(float) * 2 * 4096;
		size++;
	}
	ma_decoder_uninit(&decoder);
	*args->audioLength = audioLength;
	lockLoadingMutex();
	_loading--;
	unlockLoadingMutex();
	free(args->file);
	free(args);
	return NULL;
}

void loadAudio(Audio * audio, char *file)
{
	audio->size = 0;
	struct decodeAudioArgs *args = malloc(sizeof(struct decodeAudioArgs));
	args->audioLength = &audio->size;
	args->buffer = (void*)&audio->data;
	args->file = malloc(100);
	strcpy(args->file, file);
	#ifdef __unix
	createThread((void *(*)(void *))decodeAudio, args);
	#else
	createThread((DWORD WINAPI *(*)(void *))decodeAudio, args);
	#endif
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	// music
	if (_musicPlaying && _pMusic && _pMusic->size)
	{
		int tmpFrameCount = _musicFrameCount - _framesOffset*_musicSpeed;
		if (_pMusic->size > 0 && _pMusic->size > tmpFrameCount && tmpFrameCount > 0) 
		{
			static float smoothing = 0;
			for (int i = 0; i < frameCount * 2; i++)
			{
				((float *)pOutput)[i] = _pMusic->data[(int)(i * _musicSpeed + tmpFrameCount * 2)] * _musicVolume;
				if(_musicSpeed < 1)
				{
					smoothing = (((float *)pOutput)[i] + smoothing) / 2;
					((float *)pOutput)[i] = smoothing;
				}
			}
		}
		else if (_musicLoops && _pMusic->size > 0)
			_musicFrameCount = _musicFrameCount % _pMusic->size;
		_musicFrameCount += frameCount * _musicSpeed;

		if(_musicPreviewTimer > 0)
		{
			_musicPreviewTimer -= frameCount / (48000.0*1);
			if(_musicPreviewTimer <= 0)
			{
				stopMusic();
			}
		}
	}
	//menu music
	if (_menuMusic.size > 0 && _menuMusic.size > _menuMusicFrameCount && _menuMusicFrameCount > 0)
	{
		for (int i = 0; i < frameCount * 2; i++)
		{
			((float *)pOutput)[i] += _menuMusic.data[i + _menuMusicFrameCount * 2] * _musicVolume * (1 - _musicPlaying) * _playMenuMusic;
		}
	}
	if (_menuMusic.size != 0)
		_menuMusicFrameCount = _menuMusicFrameCount % _menuMusic.size;
	_menuMusicFrameCount += frameCount;

	// sound effects
	for (int i = 0; i < frameCount * 2; i++)
	{
		((float *)pOutput)[i] += _pEffectsBuffer[(i + _effectOffset) % (EFFECT_BUFFER_SIZE/sizeof(float))] * _audioEffectVolume;
		_pEffectsBuffer[(i + _effectOffset) % (EFFECT_BUFFER_SIZE/sizeof(float))] = 0;
	}
	_effectOffset += frameCount * 2;

	(void)pInput;
}

void audioInit()
{
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); // 4 second long buffer
	loadAudio(&_hitSe, "assets/hit.wav");
	loadAudio(&_missHitSe, "assets/missHit.wav");

	loadAudio(&_missSe, "assets/missHit.wav");
	loadAudio(&_buttonSe, "assets/button.mp3");

	loadAudio(&_clickPressSe, "assets/clickPress.mp3");
	loadAudio(&_clickReleaseSe, "assets/clickRelease.mp3");

	loadAudio(&_finishSe, "assets/finish.mp3");
	loadAudio(&_failSe, "assets/missHit.wav");

	_musicFrameCount = 0;
	_musicLoops = false;
	_musicHead = 0;
	loadAudio(&_menuMusic, "assets/menuMusic.mp3");

	_musicPlaying = false;
	_deviceConfig = ma_device_config_init(ma_device_type_playback);
	_deviceConfig.playback.format = ma_format_f32;
	_deviceConfig.playback.channels = 2;
	_deviceConfig.sampleRate = 48000;
	_deviceConfig.dataCallback = data_callback;
	_deviceConfig.periodSizeInMilliseconds = 1;
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

void loadMusic(Audio *music, char * file, float previewOffset)
{
	_musicPlaying = false;
	if (!file)
		return;
	
	if (music->size == 0)
		loadAudio(music, file);
	
	_musicPreviewOffset = previewOffset;
	_pMusic = music;
}

bool endOfMusic()
{
	if (_pMusic->size <= _musicFrameCount)
		return true;
	return false;
}

void playAudioEffect(Audio effect)
{
	int size = fmin(effect.size, EFFECT_BUFFER_SIZE/sizeof(float));
	for (int i = 0; i < size; i++)
	{
		_pEffectsBuffer[(i + _effectOffset) % (EFFECT_BUFFER_SIZE/sizeof(float))] += effect.data[i];
	}
}

void startMusic()
{
	_musicPlaying = true;
	_musicFrameCount = 1;
	_playMenuMusic = false;
}

void stopMusic()
{
	_musicFrameCount = 0;
	_musicHead = 0;
	_musicPlaying = false;
	_playMenuMusic = true;
}

void setMusicFrameCount()
{
	_musicFrameCount = _musicHead * 48000;
}