#ifndef GAME_MAIN
#define GAME_MAIN

#define MAXFPS 360.0

#ifdef EXTERN_MAIN

    extern bool _disableLoadingScreen, _isKeyPressed;
    extern int _loading;
    extern float _loadingFade, _transition;
    extern char _playerName[100];

#endif

int getWidth();
int getHeight();
void exitGame();

#endif