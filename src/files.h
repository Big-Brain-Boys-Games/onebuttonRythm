#ifndef GAME_FILE_HANDLING
#define GAME_FILE_HANDLING

#include "shared.h"
#include "gameplay.h"
#include <stdbool.h>
#include "include/raylib.h"

struct Map{
	char * folder;
	char * name;
	char * creator;
	int difficulty;
	int bpm;
	int zoom;
	int offset;
	char * musicFile;
	Texture2D image;
};
typedef struct Map Map;

struct Settings{
	int zoom;
	int volumeGlobal;
	int volumeMusic;
	int volumeSoundEffects;
};
typedef struct Settings Settings;

enum FilePart{fpNone, fpName, fpCreator, fpDifficulty, fpBPM, fpMusicFile, fpZoom, fpOffset, fpNotes};
enum SettingsPart{spNone, spZoom, spVolGlobal, spVolMusic, spVolSE};

Map loadMapInfo(char * file);
void saveFile (int noteAmount);
void loadMap(int fileType);
void unloadMap();
void saveScore();
bool readScore(Map * map, int *score, int * combo);
void makeMap(Map * map);
void addZipMap(char * file);
void makeMapZip(Map * map);

#endif