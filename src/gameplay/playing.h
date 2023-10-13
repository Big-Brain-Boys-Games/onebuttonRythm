#ifndef GAME_PLAYING
#define GAME_PLAYING

#include <stdbool.h>

#ifdef EXTERN_PLAYING

    extern float _health = 50;
    extern int _missPenalty = 5;
    extern int _hitPoints = 5;
#endif


void fCountDown(bool reset);
void fPlaying(bool reset);
void fFail(bool reset);
void fEndScreen(bool reset);

#endif