#ifndef GAME_HELPER_FUNCTIONS
#define GAME_HELPER_FUNCTIONS

float noLessThanZero(float var);
void printAllnotes();
#include "../deps/raylib/src/raylib.h"

<<<<<<< HEAD
typedef struct{
    Vector2 vec;
    float time;
}Frame;

typedef struct{
=======
typedef struct
{
>>>>>>> dd1f5a10a46073ff4e3fc120615d7446b9f59070
    float time;
    void ***hitSE;
    int *hitSE_Length;
    Texture texture;
<<<<<<< HEAD
    char * hitSE_File;
    char * texture_File;
    Frame * anim;
    int animSize;
}Note;
=======
    char *hitSE_File;
    char *texture_File;
} Note;
>>>>>>> dd1f5a10a46073ff4e3fc120615d7446b9f59070

float fDistance(float x1, float y1, float x2, float y2);
double clamp(double d, double min, double max);
int findClosestNote(Note arr[], int n, float target);

#endif