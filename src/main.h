#ifndef GAME_MAIN
#define GAME_MAIN


#ifdef EXTERN_MAIN

    extern bool _disableLoadingScreen, _isKeyPressed;
    extern int _loading;
    extern float _loadingFade, _transition;
    extern char _playerName[100];

#endif

int getWidth();
int getHeight();
void setFakeResolution(int x, int y);
void exitGame();

#endif