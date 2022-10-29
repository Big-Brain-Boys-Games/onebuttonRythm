#ifndef GAME_AUDIO_INTERFACE
#define GAME_AUDIO_INTERFACE

#include <stdbool.h>

typedef struct Audio
{
    float * data;
    int size;
} Audio;

#ifdef EXTERN_AUDIO
    extern double _musicHead, _musicSpeed;
    extern bool _musicPlaying, _musicLoops, _playMenuMusic;

    extern int _musicFrameCount;
    extern float _musicPreviewOffset;

    extern Audio _hitSe;
    extern Audio _missHitSe;
    extern Audio _missSe;
    extern Audio _buttonSe;
    extern Audio _clickPressSe;
    extern Audio _clickReleaseSe;
    extern Audio _failSe;
    extern Audio _finishSe;
    extern Audio _menuMusic;

    extern Audio *_pMusic;

    extern float _musicVolume, _audioEffectVolume;
    extern int _framesOffset;

#endif

int getSamplePosition(float time);
float getMusicDuration();
float getMusicPosition();
void fixMusicTime();
void setMusicStart();
void randomMusicPoint();
void audioInit();
void loadMusic(Audio *music, char * file, float previewOffset);
void loadAudio(Audio * audio, char *file);
bool endOfMusic();
void playAudioEffect(Audio audio);
void startMusic();
void stopMusic();
void setMusicFrameCount();

#endif