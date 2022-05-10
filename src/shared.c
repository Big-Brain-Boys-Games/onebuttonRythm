#include "shared.h"
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern float * _pNotes;
extern int _amountNotes;

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
		printf("note %.2f\n", _pNotes[i]);
	}
}

//TODO think of a better name because f used in front of a function is for gameplay functions
float fDistance(float x1, float y1, float x2, float y2)
{
	float x = x1 - x2;
	float y = y1 - y2;
	return sqrtf(fabs(x * x + y * y));
}

double clamp(double d, double min, double max) {
  const double t = d < min ? min : d;
  return t > max ? max : t;
}

float findClosest(float arr[], int n, float target)
{
    // Corner cases
    if (target <= arr[0])
        return arr[0];
    if (target >= arr[n - 1])
        return arr[n - 1];
 
    // Doing binary search
    int i = 0, j = n, mid = 0;
    while (i < j) {
        mid = (i + j) / 2;
        if (arr[mid] == target)
            return arr[mid];
 
        /* If target is less than array element,
            then search in left */
        if (target < arr[mid]) {
 
            // If target is greater than previous
            // to mid, return closest of two
            if (mid > 0 && target > arr[mid - 1])
                return getClosest(arr[mid - 1],
                                  arr[mid], target);
 
            /* Repeat for left half */
            j = mid;
        }
 
        // If target is greater than mid
        else {
            if (mid < n - 1 && target < arr[mid + 1])
                return getClosest(arr[mid],
                                  arr[mid + 1], target);
            // update i
            i = mid + 1;
        }
    }
 
    // Only single element left after search
    return arr[mid];
}
 
// Method to compare which one is the more close.
// We find the closest by taking the difference
// between the target and both values. It assumes
// that val2 is greater than val1 and target lies
// between these two.
float getClosest(float val1, float val2, float target)
{
    if (target - val1 >= val2 - target)
        return val2;
    else
        return val1;
}