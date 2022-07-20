#ifndef GAME_FILE_HANDLING
#define GAME_FILE_HANDLING

// #include "shared.h"
#include <stdbool.h>
#include "../deps/raylib/src/raylib.h"

struct Map{
	int id;
	char * folder;
	char * name;
	char * artist;
	char * mapCreator;
	int difficulty;
	int bpm;
	int zoom;
	int offset;
	float beats;
	float musicPreviewOffset;
	char * imageFile;
	char * musicFile;
	void * music;
	int musicLengthFrames;
	int musicLength;
	Texture2D image;
	Image cpuImage;
};
typedef struct Map Map;

typedef struct{
	char * file;
	Texture texture;
	int uses;
}CustomTexture;

typedef struct{
	char * file;
	void * sound;
	int length;
	int uses;
}CustomSound;

struct Settings{
	int zoom;
	int volumeGlobal;
	int volumeMusic;
	int volumeSoundEffects;
	double offset;
};
typedef struct Settings Settings;

enum FilePart{fpNone, fpID, fpName, fpArtist, fpMapCreator, fpDifficulty, fpBPM, fpImage, fpMusicFile, fpMusicLength, fpMusicPreviewOffset, fpZoom, fpOffset, fpBeats, fpNotes};
enum SettingsPart{spNone, spName, spZoom, spVolGlobal, spVolMusic, spVolSE, spOffset};

void initFolders();
Map loadMapInfo(char * file);
void saveFile (int noteAmount);
void loadMap();
void unloadMap();
void freeMap(Map * map);
void freeNotes();
void saveScore();
bool readScore(Map * map, int *score, int * combo, float * accuracy);
void makeMap(Map * map);
void addZipMap(char * file);
void makeMapZip(Map * map);
void loadSettings();
void saveSettings ();
CustomTexture * addCustomTexture(char * file);
void removeCustomTexture(char * file);
void freeAllCustomTextures ();
CustomSound * addCustomSound(char * file);
void removeCustomSound(char * file);
void freeAllCustomSounds ();

#endif