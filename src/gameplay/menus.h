#ifndef GAME_MENUS
#define GAME_MENUS

#include "../../deps/raylib/src/raylib.h"

#include "gameplay.h"
#include <stdbool.h>

#ifdef EXTERN_MENUS

    extern Modifier *_activeMod[100];

    extern Modifier _mods[];

    extern bool _mapRefresh;
    extern bool _anyUIButtonPressed;

#endif



enum CSS_Type {
    css_text,
    css_image,
    css_rectangle,
    css_toggle,
    css_button,
    css_buttonNoSprite,
    css_textbox,
    css_numberbox,
    css_slider,
    css_container,
    css_array,
    css_array_element
};

typedef struct CSS_Variable{
    char * name;
    int nameCharBits;
    char * value;
} CSS_Variable;

enum CSS_Image_State{
    CSSImage_notLoaded,
    CSSImage_loading,
    CSSImage_loaded,
    CSSImage_rank,
    CSSImage_error
};

typedef struct CSS_Image{
    char * file;
    enum CSS_Image_State state;
    Texture2D texture;
    bool * stop;
    bool isCopy;
} CSS_Image;

typedef struct CSS_Array_Child{
    bool enabled;
    float x;
    float y;
    char * key;
    int index;
} CSS_Array_Child;

typedef struct CSS_Object{
    char * name;
    int nameCharBits;
    char * parent;
    int  parentObj;
    int * children;
    int childrenCount;
    enum CSS_Type type;
    int drawTick;
    char * text;
    char * hintText;
    Color color;

    CSS_Image image;
    float opacity;
    float fontSize;
    float x, y, width, height;
    float paddingX, paddingY;
    bool selected;
    int value;
    int max, min;
    bool active, scrollable;
    float scrollValue;
    char * loadFile;
    char * makeActive;
    int makeActiveObj;
    char * command;
    float hoverTime;
    float growOnHover;
    float rotateOnHover;
    bool textClipping;
    bool keepAspectRatio;
    bool usesVariable;
    bool usesVariableHint;
    bool centered;
    bool pressedOn;

    bool arrayChild;
    int arrayObj;
    CSS_Array_Child * arrayChildren;
    int arrayChildCount;
} CSS_Object;

typedef struct CSS{
    CSS_Object * objects;
    int objectCount;
    CSS_Variable * variables;
    int variableCount;
    char * file;
} CSS;

void drawCSS(char * file);
void loadCSS(char * fileName);
CSS_Object * makeCSS_ObjectClone(CSS_Object object);
CSS_Object getCSS_Object(char * name);
CSS_Object * getCSS_ObjectPointer(char * name);
CSS_Variable * getCSS_Variable(char * name);
void setCSS_Variable(char * name, char * value);
void setCSS_VariableInt(char * name, int value);
bool UIBUttonPressed(char * name);
void fCSSPage(bool reset);
void UITextBox(char * variable, char * name);
int UIValueInteractable(int variable, char * name);
bool UISelected(char * name);
void UISetActive(char * name, bool active);

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