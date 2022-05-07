#ifndef GAME_FILE_HANDLING
#define GAME_FILE_HANDLING

#ifndef MAINC
#include "shared.h"
#include "gameplay.h"
#endif
#include <stdbool.h>
#include "include/raylib.h"

struct Map{
	char * folder;
	char * name;
	char * creator;
	int difficulty;
	int bpm;
	Texture2D image;
};

typedef struct Map Map;


enum FilePart{fpNone, fpName, fpCreator, fpDifficulty, fpBPM, fpNotes};

Map loadMapInfo(char * file);
void saveFile (int noteAmount);
void loadMap(int fileType);
void unloadMap();
void saveScore();
bool readScore(Map * map, int *score, int * combo);

#endif