#ifndef GAME_HELPER_FUNCTIONS
#define GAME_HELPER_FUNCTIONS

float noLessThanZero(float var);
void printAllnotes();
#include "../deps/raylib/src/raylib.h"

typedef struct{
    Vector2 vec;
    float time;
}Frame;

typedef struct{
    float time;
    void *** hitSE;
    int * hitSE_Length;
    Texture texture;
    char * hitSE_File;
    char * texture_File;
    Frame * anim;
    int animSize;
}Note;

float fDistance(float x1, float y1, float x2, float y2);
double clamp(double d, double min, double max);
float findClosest(Note arr[], int n, float target);
float getClosest(float val1, float val2, float target);



#endif