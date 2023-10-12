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

	//don't get saved with map V
	int highscore;
	int combo;
	int misses;
	int rank;
	float accuracy;
} Map;

#ifdef EXTERN_FILES
extern Map * _paMaps;
extern int _mapsCount;
#endif

typedef struct Settings{
	int zoom;
	bool useMapZoom;
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
	bool editorTestZoom;
	float backgroundDarkness;
	float customNoteHeigth;
	float noteOverlap;
} Settings;

enum FilePart{
	fpNone,
	fpID,
	fpName,
	fpArtist,
	fpMapCreator,
	fpDifficulty,
	fpBPM,
	fpImage,
	fpMusicFile,
	fpMusicLength,
	fpMusicPreviewOffset,
	fpZoom,
	fpOffset,
	fpBeats,
	fpTimeSignatures,
	fpNotes
};


enum SettingsPart{
	spNone,
	spName,
	spZoom,
	spUseMapZoom,
	spVolGlobal,
	spVolMusic,
	spVolSE,
	spOffset,
	spNoteSize,
	spResX,
	spResY,
	spFS,
	spAnimations,
	spCustomAssets,
	spEditorTestZoom,
	spBackgroundDarkness,
	spCustomNoteHeight,
	spNoteOverlap
};

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
void readScore(Map * map);
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