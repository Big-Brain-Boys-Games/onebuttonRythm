#ifndef GAME_FILE_HANDLING
#define GAME_FILE_HANDLING

#ifndef MAINC
#include "shared.h"
#include "gameplay.h"
#endif

struct Map{
	char * folder;
	char * name;
	char * creator;
	int difficulty;
	int bpm;
	Texture2D image;
};

typedef struct Map Map;
//TODO add support for more maps
Map _pMaps [100];

void *_pMusic;

FILE * _pFile;

enum FilePart{fpNone, fpName, fpCreator, fpDifficulty, fpBPM, fpNotes};

void data_callback(ma_device *pDevice, void *pOutput, const void *pInput, ma_uint32 frameCount);
Map loadMapInfo(char * file);
void saveFile (int noteAmount);
void * loadAudio(char * file, ma_decoder * decoder, int * audioLength);
void loadMusic(char * file);
void loadMap(int fileType);
void saveScore();
bool readScore(char * map, int *score, int * combo);

#endif