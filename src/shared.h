#ifndef GAME_HELPER_FUNCTIONS
#define GAME_HELPER_FUNCTIONS

float noLessThanZero(float var);
void printAllnotes();

float fDistance(float x1, float y1, float x2, float y2);
double clamp(double d, double min, double max);
float findClosest(float arr[], int n, float target);
float getClosest(float val1, float val2, float target);
#endif