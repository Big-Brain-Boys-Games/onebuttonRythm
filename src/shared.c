#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define EXTERN_GAMEPLAY
#define EXTERN_DRAWING
#define EXTERN_AUDIO

// #include "shared.h"

#include "audio.h"
#include "files.h"
#include "drawing.h"
#include "main.h"
#include "gameplay/gameplay.h"


float noLessThanZero(float var)
{
    if (var < 0)
        return 0;
    return var;
}

void printAllNotes()
{
    for (int i = 0; i < _amountNotes; i++)
    {
        printf("note %.2f\n", _papNotes[i]->time);
    }
}

// TODO think of a better name because f used in front of a function is for gameplay functions
float fDistance(float x1, float y1, float x2, float y2)
{
    float x = x1 - x2;
    float y = y1 - y2;
    return sqrtf(fabs(x * x + y * y));
}

double clamp(double d, double min, double max)
{
    const double t = d < min ? min : d;
    return t > max ? max : t;
}

int findClosestNote(Note ** arr, int n, float target)
{
    if(n == 0)
        return -1;
        
    // Corner cases
    if (target <= arr[0]->time)
    {
        return 0;
    }

    if (target >= arr[n - 1]->time)
    {
        return n - 1;
    }

    // Doing binary search
    int i = 0, j = n, mid = 0;
    while (i < j)
    {
        mid = (i + j) / 2;
        if (arr[mid]->time == target)
        {
            return mid;
        }

        /* If target is less than array element,
            then search in left */
        if (target < arr[mid]->time)
        {
            // If target is greater than previous
            // to mid, return closest of two
            if (mid > 0 && target > arr[mid - 1]->time)
            {
                float val1 = arr[mid -1]->time;
                float val2 = arr[mid]->time;
                if (target - val1 >= val2 - target)
                    return mid;
                else
                    return mid -1;
                // return getClosest(arr[mid - 1]->time, arr[mid]->time, target);
            }

            /* Repeat for left half */
            j = mid;
        }

        // If target is greater than mid
        else
        {
            if (mid < n - 1 && target < arr[mid + 1]->time)
            {
                // return getClosest(arr[mid]->time, arr[mid + 1]->time, target);
                float val2 = arr[mid + 1]->time;
                float val1 = arr[mid]->time;
                if (target - val1 >= val2 - target)
                    return mid + 1;
                else
                    return mid;
            }

            // update i
            i = mid + 1;
        }
    }

    // Only single element left after search
    return mid;
}

void MakeNoteCopy(Note src, Note * dest)
{
    *dest = src;

    if(src.hitSE_File != 0)
    {
        dest->hitSE_File = malloc(100);
        strncpy(dest->hitSE_File, src.hitSE_File, 100);

        dest->custSound = addCustomSound(src.custSound->file);
    }

    if(src.texture_File != 0)
    {
        dest->texture_File = malloc(100);
        strncpy(dest->texture_File, src.texture_File, 100);

        dest->custTex = addCustomTexture(src.custTex->file);
    }

    if(src.animSize)
    {
        dest->anim = malloc(src.animSize*sizeof(Frame));
        memcpy(dest->anim, src.anim, sizeof(Frame)*src.animSize);
    }
}

int rankCalculation(int score, int combo, int misses, float accuracy)
{
    if(!_amountNotes)
        return 0;
    
    float rank = 100*(1-accuracy);

    rank *= 1 - ((float)misses / _amountNotes);
    rank *= 1 + ((float)combo / _amountNotes) * 0.5;

    if(rank > 6)
        rank = 6;
    
    return (6.0/100)*rank; //map output from 100 to 6 ranks
}

float musicTimeToAnimationTime(double time)
{
    // time += _hitPart*_scrollSpeed/2;

    float size = 5;

    time -= _musicHead;
    time /= size;

    return time+0.5;
}

double animationTimeToMusicTime(float time)
{
    float size = 5;

    time -= 0.5;
    time *= size;
    time += _musicHead;

    return time;
}

Vector2 animationKey(Frame * anim, int size, float time)
{
    int index = size-1;
    
    for(int key = 0; key < size; key++)
    {
        if(anim[key].time > time)
        {
            index = key;
            break;
        }
    }

    int frame1 = index-1;
    int frame2 = index;
    if(frame1 < 0)
        frame1 = 0;

    float blendFactor = 1;
    
    if(frame1 != frame2)
    {
        float timeDifference = anim[frame2].time - anim[frame1].time;
        blendFactor = (time - anim[frame1].time) / timeDifference;
    }

    Vector2 vec;

    vec.x = anim[frame1].vec.x + (anim[frame2].vec.x - anim[frame1].vec.x)*blendFactor;
    vec.y = anim[frame1].vec.y + (anim[frame2].vec.y - anim[frame1].vec.y)*blendFactor;

    return vec;
}