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