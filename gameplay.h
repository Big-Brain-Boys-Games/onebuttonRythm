#ifndef GAME_GAMEPLAY
#define GAME_GAMEPLAY

#ifndef MAINC
#include "shared.h"
#include "drawing.h"
#endif

//Where is the current audio
float _musicHead = 0;
float _scrollSpeed = 0.6;
int _noteIndex = 0, _amountNotes =0 ;
bool _musicPlaying;
bool _noBackground = false;
float _health = 50;
int _score= 0;
int _musicFrameCount = 0;
int _musicLength = 0;
int bpm = 100;
float _maxMargin = 0.1;
int _hitPoints = 5;
int _missPenalty = 10;
char *_pMap = "testMap";
float _fadeOut = 0;

void *_pHitSE;
int _hitSE_Size;

void *_pMissHitSE;
int _missHitSE_Size;

void *_pMissSE;
int _missSE_Size;

void *_pEffectsBuffer;
int _effectOffset;

//TODO probably move this to drawing?
Color _fade = WHITE;

//Timestamp of all the notes
float * _pNotes;
void (*_pNextGameplayFunction)();
void (*_pGameplayFunction)();

bool endOfMusic();
void resetBackGround();
void playAudioEffect();
void startMusic();
void StopMusic();
bool mouseInRect(Rectangle rect);
void removeNote(int index);
void newNote(float time);
void fMainMenu();
void fEditor();
void fCountDowm();
void fRecording();
void fPlaying();
void fFail();

#endif