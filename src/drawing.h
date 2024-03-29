#ifndef GAME_DRAW_OBJECTS
#define GAME_DRAW_OBJECTS

// #include "shared.h"
#include "files.h"
// #include "gameplay.h"

#include "../deps/raylib/src/raylib.h"


#ifdef EXTERN_DRAWING

    extern Texture2D _cursorTex;
    extern Texture2D _noteTex;
    extern Texture2D _healthBarTex;
    extern Texture2D _heartTex;
    extern Texture2D _background, _menuBackground;
    extern Texture2D _buttonTile[3][3];
    extern Texture2D _iconTime;
    
    extern Font _font;

    extern Color _UIColor;

    extern Color _fade;

    extern float _hitPart;

    extern Rectangle _scissors[];
    extern int _scissorIndex;
    extern bool _scissorMode;

#endif

void startScissor(int x, int y, int width, int height);
void endScissor();
int measureText(char *text, int fontSize);
void drawText(char *str, int x, int y, int fontSize, Color color);
float musicTimeToScreen(float musicTime);
float screenToMusicTime(float x);
float noteFadeOut(float note);
void drawCursor();
void drawNote(float musicTime, Note * note, Color color, float customSize);
void dNotes();
void drawRank(int x, int y, int width, int height, int rank);
void drawTextureCorrectAspectRatio(Texture2D tex, Color color, Rectangle rect, float rotation);
void drawTextInRect(Rectangle rect, char * text, float fontSize, Color color, bool scroll);
void drawMapThumbnail(Rectangle rect, Map *map, int highScore, int combo, float accuracy, bool selected);
void drawBars();
void drawBackground();
void resetBackGround();
void drawVignette();
void drawProgressBar();
bool drawProgressBarI(bool interactable);
void drawHint(Rectangle rect, char * text);
void drawButtonPro(Rectangle rect, char * text, float fontScale, Texture2D tex, float rotation);
void drawButton(Rectangle rect, char *text, float fontScale);
void drawButtonNoSprite(Rectangle rect, char *text, float fontScale);
void drawMusicGraph(float transparent);
void drawTransition();
void initDrawing();
void drawLoadScreen();

#endif