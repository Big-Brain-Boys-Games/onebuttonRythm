#ifndef GAME_MENUS
#define GAME_MENUS

#include "../../deps/raylib/src/raylib.h"

#include "gameplay.h"
#include <stdbool.h>

#ifdef EXTERN_MENUS

    extern Modifier *_activeMod[100];

    extern Modifier _mods[];

    extern bool _mapRefresh;

#endif



enum CSS_Type {css_text, css_image, css_rectangle, css_button, css_buttonNoSprite, css_textbox, css_slider, css_container};

typedef struct CSS_Object{
    char * name;
    char * parent;
    enum CSS_Type type;
    char * text;
    Color color;
    Texture2D image;
    float opacity;
    float fontSize;
    float x, y, width, height;
    bool selected;
    int value;
    int max, min;
    bool active, scrollable;
    float scrollValue;
    char * loadFile;
    char * makeActive;
    float hoverTime;
    float growOnHover;
} CSS_Object;

typedef struct CSS{
    CSS_Object * objects;
    int count;
    char * file;
} CSS;

void loadCSS(char * fileName);
CSS_Object getCSS_Object(char * name);
CSS_Object * getCSS_ObjectPointer(char * name);
void fCSSPage(bool reset);


void fPause(bool reset);
bool mouseInRect(Rectangle rect);
void textBox(Rectangle rect, char *str, bool *selected);
void numberBox(Rectangle rect, int *number, bool *selected);
void slider(Rectangle rect, bool *selected, int *value, int max, int min);
bool drawInteractableButton(char *text, float fontScale, float x, float y, float width, float height);
bool interactableButton(char *text, float fontScale, float x, float y, float width, float height);
bool interactableButtonNoSprite(char *text, float fontScale, float x, float y, float width, float height);
void fMainMenu(bool reset);
void fSettings(bool reset);
void fNewMap(bool reset);
void fMapSelect(bool reset);
void fExport(bool reset);
void fIntro(bool reset);

#endif