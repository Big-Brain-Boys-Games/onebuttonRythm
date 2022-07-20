#include "audio.h"
#include "windowsDefs.h"
#define MINIAUDIO_IMPLEMENTATION
#include "../deps/miniaudio/miniaudio.h"

#include "files.h"
#include "gameplay.h"
#include "thread.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int _amountNotes, _loading;
extern double _musicHead;
extern float _scrollSpeed;
extern Map *_map;
extern Settings _settings;

#define EFFECT_BUFFER_SIZE 48000 * 4 * 4

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

// Where is the current audio
double _musicHead = 0;
float _musicPlaying = 0;
bool _musicLoops = true, _playMenuMusic = true;

int _musicFrameCount = 0;
int *_musicLength = 0;
float _musicPreviewOffset;

void **_pMusic = 0;

void *_pMenuMusic = 0;
int _menuMusicFrameCount = 0;
int _menuMusicLength = 0;

float _musicSpeed = 1;

int getSamplePosition(float time)
{
	return time * 48000 * 2;
}
// Get duration of music in seconds
float getMusicDuration()
{
	return *_musicLength / (float)48000;
}

float getMusicPosition()
{
	return _musicFrameCount / (float)48000;
}

void fixMusicTime()
{
	if (fabs(getMusicHead() - getMusicPosition()) > 0.1)
		_musicHead = getMusicPosition();
}

int getBarsCount()
{
	return _map->bpm * (getMusicDuration()-_map->offset/1000.0) / 60 / 4;
}

int getBeatsCount()
{
	return _map->bpm * (getMusicDuration()-_map->offset/1000.0) / 60;
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
	float startTime = (float)clock()/CLOCKS_PER_SEC;

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
		return NULL;
		// exit(0);
	}
	int lastFrame = -1;
	ma_decoder_get_length_in_pcm_frames(&decoder, &audioLength);
	*args->buffer = calloc(sizeof(float) * 2 * 2, audioLength);
	void *pCursor = *args->buffer;
	int size = 0;
	lastFrame = -1;

	float middleTime = (float)clock()/CLOCKS_PER_SEC;

	printf("loading and allocating music file %s, took %.2f seconds\n", args->file, middleTime - startTime);
	while (decoder.readPointerInPCMFrames != lastFrame)
	{
		lastFrame = decoder.readPointerInPCMFrames;
		ma_result result = ma_decoder_read_pcm_frames(&decoder, pCursor, 4096, NULL);
		pCursor += sizeof(float) * 2 * 4096;
		size++;
	}
	float endTime = (float)clock()/CLOCKS_PER_SEC;
	printf("reading music file %s, took %.2f seconds\n", args->file, endTime - middleTime);
	printf("total music file %s, took %.2f seconds\n", args->file, endTime - startTime);
	ma_decoder_uninit(&decoder);
	*args->audioLength = audioLength;
	lockLoadingMutex();
	_loading--;
	unlockLoadingMutex();
	free(args->file);
	free(args);
	return NULL;
}

void loadAudio(void **buffer, char *file, int *audioLength)
{
	static int threadIndex = 0;

	*audioLength = 0;
	struct decodeAudioArgs *args = malloc(sizeof(struct decodeAudioArgs));
	args->audioLength = audioLength;
	args->buffer = buffer;
	args->file = malloc(strlen(file) + 5);
	strcpy(args->file, file);
	createThread((void *(*)(void *))decodeAudio, args);
	threadIndex++;
}

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
{
	float globalVolume = _settings.volumeGlobal / 100.0;
	float musicVolume = _settings.volumeMusic / 100.0 * globalVolume;
	float audioEffectVolume = _settings.volumeSoundEffects / 100.0 * globalVolume;

	// music
	if (_musicPlaying && _pMusic && *_pMusic && _musicLength)
	{
		int tmpFrameCount = _musicFrameCount + _settings.offset*48000;
		if (*_musicLength > 0 && *_musicLength > tmpFrameCount && tmpFrameCount > 0)
		{
			for (int i = 0; i < frameCount * 2; i++)
			{
				float value = 0;
				int sampleCount = (int)(_musicSpeed * 100);
				// for(int j = 0; j < sampleCount; j++)
				// {
				value += ((float *)*_pMusic)[(int)(i * (double)_musicSpeed + tmpFrameCount * 2)] * musicVolume;
				// }
				((float *)pOutput)[i] = value;
			}
		}
		else if (_musicLoops && *_musicLength > 0)
			_musicFrameCount = _musicFrameCount % *_musicLength;
		_musicFrameCount += frameCount * _musicSpeed;
	}
	if (_pMenuMusic && _menuMusicLength > 0 && _menuMusicLength > _menuMusicFrameCount && _menuMusicFrameCount > 0)
	{
		for (int i = 0; i < frameCount * 2; i++)
		{
			((float *)pOutput)[i] += ((float *)_pMenuMusic)[i + _menuMusicFrameCount * 2] * musicVolume * (1 - _musicPlaying) * _playMenuMusic;
		}
	}
	if (_menuMusicLength != 0)
		_menuMusicFrameCount = _menuMusicFrameCount % _menuMusicLength;
	_menuMusicFrameCount += frameCount;
	// sound effects

	// int tmpFrameCount = _effectOffset + _settings.offset&
	for (int i = 0; i < frameCount * 2; i++)
	{
		((float *)pOutput)[i] += ((float *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] * audioEffectVolume;
		((float *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] = 0;
	}
	_effectOffset += frameCount * 2;

	(void)pInput;
}

void audioInit()
{
	// todo do this smarter
	_pEffectsBuffer = calloc(sizeof(char), EFFECT_BUFFER_SIZE); // 4 second long buffer
	loadAudio(&_pHitSE, "assets/hit.mp3", &_hitSE_Size);
	loadAudio(&_pMissHitSE, "assets/missHit.mp3", &_missHitSE_Size);
	printf("%s \t %p\n", "assets/missHit.mp3", _pMissSE);
	loadAudio(&_pMissSE, "assets/missHit.mp3", &_missSE_Size);
	loadAudio(&_pButtonSE, "assets/button.mp3", &_buttonSE_Size);

	loadAudio(&_pClickPress, "assets/clickPress.mp3", &_clickPressSE_Size);
	loadAudio(&_pClickRelease, "assets/clickRelease.mp3", &_clickReleaseSE_Size);

	loadAudio(&_pFinishSE, "assets/finish.mp3", &_finishSE_Size);
	loadAudio(&_pFailSE, "assets/missHit.mp3", &_failSE_Size);

	_musicFrameCount = 0;
	_musicLength = 0;
	_musicLoops = false;
	_musicHead = 0;
	_menuMusicLength = 0;
	loadAudio(&_pMenuMusic, "assets/menuMusic.mp3", &_menuMusicLength);
	_menuMusicLength = 0;

	_musicPlaying = false;
	_deviceConfig = ma_device_config_init(ma_device_type_playback);
	_deviceConfig.playback.format = ma_format_f32;
	_deviceConfig.playback.channels = 2;
	_deviceConfig.sampleRate = 48000;
	_deviceConfig.dataCallback = data_callback;
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

void loadMusic(Map *map)
{
	_musicPlaying = false;
	char str[100];
	if (map->folder == NULL || map->musicFile == NULL)
		return;
	strcpy(str, "maps/");
	strcat(str, map->folder);
	strcat(str, map->musicFile);
	if (map->music == 0)
		loadAudio(&map->music, str, &map->musicLengthFrames);
	_musicLength = &map->musicLengthFrames;
	_musicPreviewOffset = map->musicPreviewOffset;
	_pMusic = &map->music;
}

bool endOfMusic()
{
	if (*_musicLength < _musicFrameCount)
		return true;
	return false;
}

void playAudioEffect(void *effect, int size)
{
	if (!effect || !size)
		return;
	for (int i = 0; i < size; i++)
	{
		((float *)_pEffectsBuffer)[(i + _effectOffset) % (48000 * 4)] += ((float *)effect)[i];
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