#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define EXTERN_GAMEPLAY

// #include "shared.h"

#include "audio.h"
#include "files.h"
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
        dest->hitSE_File = malloc(strlen(src.hitSE_File));
        strcpy(src.hitSE_File, dest->hitSE_File);

        dest->custSound = addCustomSound(src.custSound->file);
    }

    if(src.texture_File != 0)
    {
        dest->texture_File = malloc(strlen(src.texture_File));
        strcpy(src.texture_File, dest->texture_File);

        dest->custTex = addCustomTexture(src.custTex->file);
    }

    if(src.animSize)
    {
        dest->anim = malloc(src.animSize);
        memcpy(dest->anim, src.anim, sizeof(Frame)*src.animSize);
    }
}