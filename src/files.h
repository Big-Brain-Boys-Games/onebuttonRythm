#ifndef GAME_FILE_HANDLING
#define GAME_FILE_HANDLING



#include <stdbool.h>
#include "../deps/raylib/src/raylib.h"

#include "audio.h"

typedef struct Map{
	int id;
	char * folder;
	char * name;
	char * artist;
	char * mapCreator;
	int difficulty;
	int bpm;
	int zoom;
	int offset;
	int beats;
	float musicPreviewOffset;
	char * imageFile;
	char * musicFile;
	Audio music;
	int musicLength;
	Texture2D image;
	Image cpuImage;
} Map;

#ifdef EXTERN_FILES
extern Map * _paMaps;
#endif

struct Settings{
	int zoom;
	int volumeGlobal;
	int volumeMusic;
	int volumeSoundEffects;
	int noteSize;
	int resolutionX;
	int resolutionY;
	int fullscreen;
	double offset;
	bool animations;
	bool customAssets;
};
typedef struct Settings Settings;

enum FilePart{fpNone, fpID, fpName, fpArtist, fpMapCreator, fpDifficulty, fpBPM, fpImage, fpMusicFile, fpMusicLength, fpMusicPreviewOffset, fpZoom, fpOffset, fpBeats, fpTimeSignatures, fpNotes};
enum SettingsPart{spNone, spName, spZoom, spVolGlobal, spVolMusic, spVolSE, spOffset, spNoteSize, spResX, spResY, spFS, spAnimations, spCustomAssets};

#include "shared.h"


void initFolders();
Map loadMapInfo(char * file);
void saveMap ();
void loadMap();
void unloadMap();
void freeMap(Map * map);
void freeNote(Note * note);
void freeNotes();
void saveScore();
void readScore(Map * map, int *score, int * combo, int * misses, float * accuracy, int * rank);
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