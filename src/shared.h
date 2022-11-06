#ifndef GAME_HELPER_FUNCTIONS
#define GAME_HELPER_FUNCTIONS

float noLessThanZero(float var);
void printAllnotes();
#include "../deps/raylib/src/raylib.h"

typedef struct{
    Vector2 vec;
    float time;
}Frame;

#include "audio.h"

typedef struct{
	char * file;
	Texture texture;
	int uses;
}CustomTexture;

typedef struct{
	char * file;
	Audio sound;
	int uses;
}CustomSound;

typedef struct{
    float time;
    CustomSound * custSound;
    CustomTexture * custTex;
    char * hitSE_File;
    char * texture_File;
    Frame * anim;
    int animSize;
    float health;
    short int hit;
}Note;

typedef struct{
    float time;
    int bpm;
    int beats;
}TimingSegment;

float fDistance(float x1, float y1, float x2, float y2);
double clamp(double d, double min, double max);
int findClosestNote(Note ** arr, int n, float target);

void MakeNoteCopy(Note src, Note * dest);
int rankCalculation(int score, int combo, int misses, float accuracy);
float musicTimeToAnimationTime(double time);
double animationTimeToMusicTime(float time);

#endif