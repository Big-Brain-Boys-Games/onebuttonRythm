#ifndef GAME_AUDIO_INTERFACE
#define GAME_AUDIO_INTERFACE

int getSamplePosition(float time);
float getMusicDuration();
float getMusicPosition();
void fixMusicTime();
int getBarsCount();
int getBeatsCount();
void setMusicStart();
void randomMusicPoint();
void * loadAudio(char * file, ma_decoder * decoder, int * audioLength)
void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount)
void audioInit();
void loadMusic(char * file);
bool endOfMusic();
void playAudioEffect(void *effect, int size);
void startMusic();
void stopMusic();
void setMusicFrameCount();


#endif