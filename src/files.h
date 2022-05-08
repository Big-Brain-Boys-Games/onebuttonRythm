#ifndef GAME_FILE_HANDLING
#define GAME_FILE_HANDLING

#include "shared.h"
#include "gameplay.h"
#include <stdbool.h>
#include "deps/raylib/src/raylib.h"

struct Map{
	char * folder;
	char * name;
	char * creator;
	int difficulty;
	int bpm;
	int zoom;
	int offset;
	char * imageFile;
	char * musicFile;
	int musicLength;
	Texture2D image;
};
typedef struct Map Map;

struct Settings{
	int zoom;
	int volumeGlobal;
	int volumeMusic;
	int volumeSoundEffects;
	float offset;
};
typedef struct Settings Settings;

enum FilePart{fpNone, fpName, fpCreator, fpDifficulty, fpBPM, fpImage, fpMusicFile, fpMusicLength, fpZoom, fpOffset, fpNotes};
enum SettingsPart{spNone, spZoom, spVolGlobal, spVolMusic, spVolSE, spOffset};

Map loadMapInfo(char * file);
void saveFile (int noteAmount);
void loadMap();
void unloadMap();
void saveScore();
bool readScore(Map * map, int *score, int * combo);
void makeMap(Map * map);
void addZipMap(char * file);
void makeMapZip(Map * map);
void loadSettings();
void saveSettings ();

#endif