

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "../../deps/raylib/src/raylib.h"


#define EXTERN_MAIN
#define EXTERN_GAMEPLAY
#define EXTERN_DRAWING
#define EXTERN_AUDIO
#define EXTERN_FILES

#include "menus.h"

#include "gameplay.h"
#include "playing.h"
#include "recording.h"
#include "editor.h"

#include "../files.h"
#include "../drawing.h"
#include "../thread.h"
#include "../main.h"

CSS * _pCSS = 0;

float _scrollValue = 0;
bool _mapRefresh = true;

#define freeArray(arr) \
	if(arr) { \
		free(arr); \
		arr = 0; \
	}

void freeCSS_Object(CSS_Object * object)
{
	if(!object)
		return;

	if(object->image.id)
	{
		if(_map && object->image.id == _map->image.id)
		{}
		else
		{
			UnloadTexture(object->image);
		}
	}

	if(object->name)
		free(object->name);
	
	if(object->text && !object->usesVariable)
		free(object->text);
}

void freeCSS(CSS * css)
{
	if(!css)
		return;
	
	for(int i = 0; i < css->objectCount; i++)
	{
		freeCSS_Object(&css->objects[i]);
	}

	free(css->objects);

	for(int i = 0; i < css->variableCount; i++)
	{
		freeArray(css->variables[i].name);
		freeArray(css->variables[i].value);
	}
	free(css->variables);
	css->variableCount = 0;
	css->objectCount = 0;
}

void drawCSS_Object(CSS_Object * object)
{
	if(!object->active)
		return;

	float scrollValue = _scrollValue;


	

	float height = getHeight();
	if(object->keepAspectRatio)
		height = getWidth();

	Rectangle rect = (Rectangle){.x=object->x*getWidth(), .y=(object->y+scrollValue)*getHeight(), .width=object->width*getWidth(), .height=object->height*height};

	

	
	bool scissorMode = false;
	if(object->parent)
	{
		CSS_Object parent = getCSS_Object(object->parent);

		if(!parent.active)
			return;

		float growAmount = parent.growOnHover * fmin(parent.hoverTime*15, 1);
		

		rect.x *= parent.width;
		rect.y *= parent.height;

		rect.x += parent.x*getWidth();
		rect.y += parent.y*getHeight();

		rect.width *= parent.width;
		rect.height *= parent.height;

		rect.x -= rect.width*growAmount/2;
		rect.y -= rect.height*growAmount/2;

		rect.width += rect.width*growAmount;
		rect.height += rect.height*growAmount;

		if(parent.type == css_container)
		{
			startScissor(parent.x*getWidth(), parent.y*getHeight(), parent.width*getWidth(), parent.height*getHeight());
			scissorMode = true;
			if(parent.scrollable)
				scrollValue = parent.scrollValue;
		}
	}

	if(mouseInRect(rect) || object->selected)
	{
		object->hoverTime = fmin(GetFrameTime()+object->hoverTime, 0.1);
	}else
		object->hoverTime = fmax(object->hoverTime-GetFrameTime(), 0);

	float growAmount = object->growOnHover * fmin(object->hoverTime*15, 1);


	rect.x -= rect.width*growAmount/2;
	rect.y -= rect.height*growAmount/2;

	rect.width += rect.width*growAmount;
	rect.height += rect.height*growAmount;

	float fontSize = object->fontSize * (1+growAmount);

	float rotation = fmin(object->hoverTime*15, 1) * object->rotateOnHover;

	switch(object->type)
	{
		case css_text:
			if(object->text)
				drawTextInRect(rect, object->text, fontSize*getWidth(), object->color, mouseInRect(rect));
			break;
		
		case css_image:
			drawTextureCorrectAspectRatio(object->image, object->color, rect, rotation);
			break;
		
		case css_rectangle:
			DrawRectangle(rect.x, rect.y, rect.width, rect.height, object->color);
			break;

		case css_button:
			if(object->image.id)
				drawButtonPro(rect, object->text, fontSize, object->image, rotation);
			else
				drawButton(rect, object->text, fontSize);

			object->selected = mouseInRect(rect) && IsMouseButtonReleased(0);
			break;

		case css_buttonNoSprite:
			drawButtonNoSprite(rect, object->text, fontSize);
			object->selected = mouseInRect(rect) && IsMouseButtonReleased(0);
			break;

		case css_textbox:
			textBox(rect, object->text, &object->selected);
			break;

		case css_numberbox:
			if(!object->selected)
				snprintf(object->text, 100, "%i", object->value);
			
			textBox(rect, object->text, &object->selected);
			
			object->value = atoi(object->text);
			break;

		case css_slider:
			slider(rect, &object->selected, &object->value, object->max, object->min);
			break;
		
		case css_container:
			object->scrollValue += GetMouseWheelMove() * .04;
			break;
	}

	if(scissorMode)
	{
		endScissor();
	}

	if(object->value > object->max)
		object->value = object->max;
	
	if(object->value < object->min)
		object->value = object->min;
		

	if((object->type == css_button || object->type == css_buttonNoSprite) && object->selected)
	{
		//button action
		if(object->makeActive)
		{
			CSS_Object * activatedObject = getCSS_ObjectPointer(object->makeActive);
			activatedObject->active = !activatedObject->active;
		}

		if(object->loadFile)
		{
			if(!strcmp(object->loadFile, "_playing_"))
			{
				_pNextGameplayFunction = &fPlaying;
				_pGameplayFunction = &fCountDown;
				fCountDown(true);
				fPlaying(true);
				_transition = 0.1;
			}else if(!strcmp(object->loadFile, "_editor_"))
			{
				_pNextGameplayFunction = &fPlaying;
				_pGameplayFunction = &fEditor;
				fEditor(true);
				_transition = 0.1;
			}else if(!strcmp(object->loadFile, "_export_"))
			{
				_pNextGameplayFunction = &fMainMenu;
				_pGameplayFunction = &fExport;
				_transition = 0.1;
			}else if(!strcmp(object->loadFile, "_mapselect_"))
			{
				_pNextGameplayFunction = _pGameplayFunction;
				_pGameplayFunction = &fMapSelect;
				_mapRefresh = true;
				_transition = 0.1;
			}else if(!strcmp(object->loadFile, "_settings_"))
			{
				_pNextGameplayFunction = _pGameplayFunction;
				_pGameplayFunction = &fSettings;
				_transition = 0.1;
			}else if(!strcmp(object->loadFile, "_newMap_"))
			{
				_pNextGameplayFunction = _pGameplayFunction;
				_pGameplayFunction = &fNewMap;
				fNewMap(true);
				_transition = 0.1;
			}else if(!strcmp(object->loadFile, "_exit_"))
			{
				exitGame();
			}else
			{
				char * file = malloc(strlen(object->loadFile)+1);
				strcpy(file, object->loadFile);
				freeCSS(_pCSS);
				free(_pCSS);
				_pCSS = 0;
				_transition = 0.1;
				loadCSS(file);
				_pGameplayFunction = &fCSSPage;
			}
			return;
		}
	}
}

void drawCSS(char * file)
{
	if(!_pCSS)
	{
		loadCSS(file);
	}

	if(strcmp(_pCSS->file, file))
	{
		freeCSS(_pCSS);
		free(_pCSS);
		_pCSS = 0;
		loadCSS(file);
	}
	
	if(IsKeyPressed(KEY_F5))
	{
		//reload
		char * file = malloc(strlen(_pCSS->file)+1);
		strcpy(file, _pCSS->file);
		freeCSS(_pCSS);
		free(_pCSS);
		loadCSS(file);
		free(file);

		if(!_pCSS)
			return;
	}

	//go through every object and draw them in order of the source	
	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		drawCSS_Object(&_pCSS->objects[i]);
	}
}

void drawContainer(char * name, int x, int y)
{
	CSS_Object * object = getCSS_ObjectPointer(name);
	CSS_Object original = *object;

	object->x = (float)x/getWidth();
	object->y = (float)y/getHeight();

	object->active = true;

	drawCSS_Object(object);

	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		if(!_pCSS->objects[i].parent)
			continue;
		
		if(object->name[0] == _pCSS->objects[i].parent[0] && !strcmp(object->name, _pCSS->objects[i].parent))
		{
			//found child
			drawCSS_Object(&_pCSS->objects[i]);
		}
	}

	*object = original;
}

char * addCSS_Variable(char * name)
{
	_pCSS->variableCount++;
	if(!_pCSS->variables)
	{
		_pCSS->variables = malloc(sizeof(CSS_Variable)*_pCSS->variableCount);
	}else
	{
		_pCSS->variables = realloc(_pCSS->variables, sizeof(CSS_Variable)*_pCSS->variableCount);
	}

	_pCSS->variables[_pCSS->variableCount-1].name = malloc(100);
	strcpy(_pCSS->variables[_pCSS->variableCount-1].name, name);
	_pCSS->variables[_pCSS->variableCount-1].value = malloc(100);
	_pCSS->variables[_pCSS->variableCount-1].value[0] = '\0';
	return _pCSS->variables[_pCSS->variableCount-1].value;
}

CSS_Variable * getCSS_Variable(char * name)
{
	for(int i = 0; i < _pCSS->variableCount; i++)
	{
		if(name[0] != _pCSS->variables[i].name[0])
			continue;

		if(!strcmp(name, _pCSS->variables[i].name))
			return &_pCSS->variables[i];
	}
	return 0;
}

void setCSS_Variable(char * name, char * value)
{
	if(!name || !value)
		return;
	
	CSS_Variable * var = getCSS_Variable(name);
	if(var)
		strcpy(var->value, value);
}

void setCSS_VariableInt(char * name, int value)
{
	if(!name)
		return;
	
	CSS_Variable * var = getCSS_Variable(name);
	if(var)
		snprintf(var->value, 100, "%i", value);
}

void loadCSS(char * fileName)
{
	FILE * file = fopen(fileName, "r");

	fseek(file, 0L, SEEK_END);
	int size = ftell(file);
	rewind(file);

	char * text = malloc(size + 1);
	int textIndex = 0;
	char ch;
	bool inText = false;
	while((ch = getc(file)) != EOF)
    {
		if(!inText && (ch == '\n' || ch == '\0' || ch == ' '))
			continue;

		if(ch == '\r') //not needed either way
			continue;
		
		if(ch == '"')
		{
			inText = !inText;
			continue;
		}
		
		text[textIndex] = ch;
		textIndex++;

		
    }
	fclose(file);

	text[textIndex] = '\0';

	size = textIndex;
	textIndex = 0;

	_pCSS = malloc(sizeof(CSS));
	_pCSS->file = malloc(strlen(fileName)+1);
	strcpy(_pCSS->file, fileName);
	_pCSS->objectCount = 0;
	_pCSS->objects = 0;
	_pCSS->variables = 0;
	_pCSS->variableCount = 0;

	for(int i = 0; i < size; i++)
	{
		if(text[i] == '#')
		{
			i++;
			CSS_Object object = {0};
			object.opacity = 1;
			object.active = true;

			for(int nameIndex = i; nameIndex < size; nameIndex++)
			{
				if(text[nameIndex] == '{')
				{
					text[nameIndex] = '\0';
					object.name = malloc(strlen(text+i)+1);
					strcpy(object.name, text+i);
					i = nameIndex+1;
					break;
				}
			}

			// printf("\n#%s {\n", object.name);

			while(text[i] != '}')
			{
				for(int varIndex = i; varIndex < size; varIndex++)
				{
					if(text[varIndex] == ':')
					{
						text[varIndex] = '\0';

						//get value for variable
						char * value = text+varIndex+1;
						char * var = text+i;

						for(int valueIndex = varIndex+1; valueIndex < size; valueIndex++)
						{
							if(text[valueIndex] == ';')
							{
								text[valueIndex] = '\0';
								i = valueIndex;
								break;
							}
						}
						
						// printf("\tvar: %s  value %s\n", var, value);
						//handle different variable types (type: text: color: image: ...)
						if(!strcmp(var, "type"))
						{
							if(!strcmp(value, "text"))
								object.type = css_text;
							
							if(!strcmp(value, "image"))
								object.type = css_image;

							if(!strcmp(value, "rectangle"))
								object.type = css_rectangle;

							if(!strcmp(value, "button"))
								object.type = css_button;

							if(!strcmp(value, "buttonNoSprite"))
								object.type = css_buttonNoSprite;

							if(!strcmp(value, "textbox"))
								object.type = css_textbox;

							if(!strcmp(value, "slider"))
								object.type = css_slider;

							if(!strcmp(value, "container"))
								object.type = css_container;

							if(!strcmp(value, "numberbox"))
								object.type = css_numberbox;
						}

						if(!strcmp(var, "content"))
						{
							if(strlen(value))
							{

								if(value[0] == '$')
								{
									object.text = addCSS_Variable(value+1);
									object.usesVariable = true;
								}else if(!strcmp(value, "_mapname_") && _map)
								{
									object.text = malloc(strlen(_map->name)+1);
									strcpy(object.text, _map->name);
								}else if(!strcmp(value, "_playerName_"))
								{
									object.text = malloc(strlen(_playerName)+1);
									strcpy(object.text, _playerName);
								}else if(!strcmp(value, "_notification_"))
								{
									object.text = malloc(strlen(_notfication)+1);
									strcpy(object.text, _notfication);
								}else {
									object.text = malloc(strlen(value)+1);
									strcpy(object.text, value);
								}
							}
						}

						if(!strcmp(var, "parent"))
						{
							if(strlen(value))
							{
								object.parent = malloc(strlen(value)+1);
								strcpy(object.parent, value);
							}
						}

						if(!strcmp(var, "makeActive"))
						{
							if(strlen(value))
							{
								object.makeActive = malloc(strlen(value)+1);
								strcpy(object.makeActive, value);
							}
						}

						if(!strcmp(var, "keepAspectRatio"))
						{
							object.keepAspectRatio = !strcmp(value, "yes");
						}

						if(!strcmp(var, "loadFile"))
						{
							if(strlen(value))
							{
								object.loadFile = malloc(strlen(value)+1);
								strcpy(object.loadFile, value);
							}
						}

						if(!strcmp(var, "active"))
						{
							object.active = !strcmp(value, "yes");
						}

						if(!strcmp(var, "scrollable"))
						{
							object.scrollable = !strcmp(value, "yes");
						}

						if(!strcmp(var, "color"))
						{
							Color colors[] = {WHITE, BLACK, MAGENTA, LIGHTGRAY, GRAY, DARKGRAY, YELLOW, GOLD, ORANGE, PINK, RED, MAROON, GREEN, LIME, DARKGREEN, SKYBLUE, BLUE, DARKBLUE, PURPLE, VIOLET, DARKPURPLE, BEIGE, BROWN, DARKBROWN};
							char * text [] = {"WHITE", "BLACK", "MEGENTA", "LIGHTGRAY", "GRAY", "DARKGRAY", "YELLOW", "GOLD", "ORANGE", "PINK", "RED", "MAROON", "GREEN", "LIME", "DARKGREEN", "SKYBLUE", "BLUE", "DARKBLUE", "PURPLE", "VIOLET", "DARKPURPLE", "BEIGE", "BROWN", "DARKBROWN"};

							int size = sizeof(colors)/sizeof(Color);
							for(int colorIndex = 0; colorIndex < size; colorIndex++)
							{
								if(!strcmp(value, text[colorIndex]))
								{
									object.color = colors[colorIndex];
									break;
								}
							}
						}

						if(!strcmp(var, "image"))
						{
							if(!strcmp(value, "_mapimage_"))
							{
								if(_map)
								{
									object.image = _map->image;
								}
							}else if(!strcmp(value, "_background_"))
							{
								object.image = _menuBackground;
							}else
							{
								object.image = LoadTexture(value);
							}
						}

						if(!strcmp(var, "opacity"))
						{
							object.opacity = atof(value);
						}

						if(!strcmp(var, "growOnHover"))
						{
							object.growOnHover = atof(value);
						}

						if(!strcmp(var, "rotateOnHover"))
						{
							object.rotateOnHover = atof(value);
						}

						if(!strcmp(var, "font-size"))
						{
							object.fontSize = atof(value)/100.0;
						}

						if(!strcmp(var, "left"))
						{
							object.x = atof(value)/100.0;
						}

						if(!strcmp(var, "top"))
						{
							object.y = atof(value)/100.0;
						}

						if(!strcmp(var, "margin-left"))
						{
							if(object.parent)
							{
								CSS_Object parent = getCSS_Object(object.parent);
								object.x = atof(value)/100.0;
							}else
							{
								if(_pCSS->objectCount < 1)
									object.x = atof(value)/100.0;
								else
									object.x = _pCSS->objects[_pCSS->objectCount-1].x + atof(value)/100.0;
							}
						}

						if(!strcmp(var, "margin-top"))
						{
							if(object.parent)
							{
								CSS_Object parent = getCSS_Object(object.parent);
								object.y = atof(value)/100.0;
							}else
							{
								if(_pCSS->objectCount < 1)
									object.y = atof(value)/100.0;
								else
									object.y = _pCSS->objects[_pCSS->objectCount-1].y + atof(value)/100.0;
							}
						}

						if(!strcmp(var, "width"))
						{
							object.width = atof(value)/100.0;
						}

						if(!strcmp(var, "height"))
						{
							object.height = atof(value)/100.0;
						}

						if(!strcmp(var, "max"))
						{
							object.max = atoi(value);
						}

						if(!strcmp(var, "min"))
						{
							object.min = atoi(value);
						}

						i++;
						break;
					}
				}
			}

			// printf("}\n");

			if(!object.text)
			{
				object.text = malloc(100);
				object.text[0] = '\0';
			}

			object.color.a = object.opacity*255;

			_pCSS->objectCount++;
			if(_pCSS->objects)
			{
				_pCSS->objects = realloc(_pCSS->objects, sizeof(CSS_Object)*_pCSS->objectCount);
			}else {
				_pCSS->objects = malloc(sizeof(CSS_Object)*_pCSS->objectCount);
			}
			_pCSS->objects[_pCSS->objectCount-1] = object;
		}
	}
}

CSS_Object * getCSS_ObjectPointer(char * name)
{
	if(!name || !_pCSS)
		return 0;
	
	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		if(_pCSS->objects[i].name[0] == name[0])
		{
			if(!strcmp(_pCSS->objects[i].name, name))
			{
				return &_pCSS->objects[i];
			}
		}
	}
	return 0;
}

CSS_Object getCSS_Object(char * name)
{
	CSS_Object * pointer = getCSS_ObjectPointer(name);
	if(pointer)
	{
		return *(pointer);
	}
	return (CSS_Object){0};
}

bool UIBUttonPressed(char * name)
{
	CSS_Object object = getCSS_Object(name);

	if (object.selected)
	{
		playAudioEffect(_buttonSe);
		return true;
	}
	return false;
}

void UITextBox(char * variable, char * name)
{
	if(!variable || !name)
		return;
	
	CSS_Object *object = getCSS_ObjectPointer(name);

	if(!object || !object->text)
		return;
	
	if(object->selected)
	{
		strcpy(variable, object->text);
	}else
	{
		strcpy(object->text, variable);
	}
}

int UIValueInteractable(int variable, char * name)
{
	if(!name)
		return variable;
	
	CSS_Object *object = getCSS_ObjectPointer(name);

	if(!object)
		return variable;
	
	if(object->selected)
	{
		return object->value;
	}else
	{
		object->value = variable;
		return variable;
	}
}

void UISetActive(char * name, bool active)
{
	if(!name)
		return;
	
	CSS_Object *object = getCSS_ObjectPointer(name);

	if(!object)
		return;
	
	object->active = active;
}


Modifier *_activeMod[100] = {0}; // we dont even have that many

Modifier _mods[] = {
	(Modifier){.id = 0, .name = "No Fail", .icon = 0, .healthMod = 0, .scoreMod = 0.2, .speedMod = 1, .marginMod = 1},
	(Modifier){.id = 1, .name = "Easy", .icon = 0, .healthMod = 0.5, .scoreMod = 0.5, .speedMod = 1, .marginMod = 2},
	(Modifier){.id = 2, .name = "Hard", .icon = 0, .healthMod = 1.5, .scoreMod = 1.5, .speedMod = 1, .marginMod = 0.5},
	(Modifier){.id = 3, .name = ".5x", .icon = 0, .healthMod = 1, .scoreMod = 0.5, .speedMod = 0.5, .marginMod = 1},
	(Modifier){.id = 4, .name = "2x", .icon = 0, .healthMod = 1, .scoreMod = 1.5, .speedMod = 1.5 /* :P */, .marginMod = 1},
};



void fCSSPage(bool reset)
{
	_musicLoops = true;
	_playMenuMusic = true;
	_musicPlaying = false;

	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);
	drawVignette();
	drawCSS(_pCSS->file);
	drawCursor();
}

void fMainMenu(bool reset)
{

	checkFileDropped();
	_musicLoops = true;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	int middle = getWidth() / 2;
	drawVignette();

	if(UIBUttonPressed("settingsButton"))
	{
		// Switching to settings
		_pGameplayFunction = &fSettings;
		_transition = 0.1;
	}

	if(UIBUttonPressed("newmapButton"))
	{
		_pGameplayFunction = &fNewMap;
		fNewMap(true);
		_transition = 0.1;
	}

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("exitButton"))
	{
		exitGame(0);
	}

	drawCSS("theme/main.css");

	drawCursor();
}

void fSettings(bool reset)
{

	static float menuScroll = 0;
	static float menuScrollSmooth = 0;
	if(reset)
	{
		menuScroll = 0;
		menuScrollSmooth = 0;
		return;
	}
	
	_musicPlaying = false;
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);
	

	drawCSS("theme/settings.css");

	_settings.volumeGlobal = UIValueInteractable(_settings.volumeGlobal, "globalVolume");
	_settings.volumeMusic = UIValueInteractable(_settings.volumeMusic, "musicVolume");
	_settings.volumeSoundEffects = UIValueInteractable(_settings.volumeSoundEffects, "effectVolume");

	_settings.zoom = UIValueInteractable(_settings.zoom, "zoomSlider");
	_settings.noteSize = UIValueInteractable(_settings.noteSize, "noteSizeSlider");
	_settings.offset = UIValueInteractable(_settings.offset, "offsetSlider");
	_settings.offset = UIValueInteractable(_settings.offset, "offsetBox");

	UITextBox(_playerName, "nameBox");

	if ( IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
		
		if(_settings.noteSize == 0)
			_settings.noteSize = 10;
		
		saveSettings();
		return;
	}

	drawCursor();
}


void fPause(bool reset)
{
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();

	dNotes();
	drawVignette();
	
	DrawRectangle(0, 0, getWidth(), getHeight(), (Color){.r = 0, .g = 0, .b = 0, .a = 128});

	drawCSS("theme/pause.css");

	drawProgressBar();

	// TODO dynamically change seperation depending on the amount of buttons?
	float middle = getWidth() / 2;

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("continueButton"))
	{
		if (_pNextGameplayFunction == &fPlaying)
		{
			_pGameplayFunction = &fCountDown;
			fCountDown(true);
		}
		else
			_pGameplayFunction = _pNextGameplayFunction;
	}

	UISetActive("retryButton", (_pNextGameplayFunction == &fPlaying || _pNextGameplayFunction == &fRecording));
	UISetActive("saveButton", (_pNextGameplayFunction == &fEditor));

	if (UIBUttonPressed("saveButton"))
	{
		saveMap();
		_pGameplayFunction = _pNextGameplayFunction;
		// gotoMainMenu(false);
	}

	if (_pNextGameplayFunction == &fPlaying && UIBUttonPressed("retryButton"))
	{
		_pGameplayFunction = fCountDown;
		_noteIndex = 0;
		_musicHead = 0;
		fCountDown(true);
		fPlaying(true);
	}

	if (_pNextGameplayFunction == &fRecording && UIBUttonPressed("retryButton"))
	{
		_pGameplayFunction = _pNextGameplayFunction;
		_noteIndex = 0;
		_amountNotes = 0;
		_musicHead = 0;
		fRecording(true);
	}

	if (UIBUttonPressed("exitButton"))
	{
		unloadMap();
		gotoMainMenu(false);
	}
	drawCursor();
}


struct mapInfoLoadingArgs
{
	int *amount;
	int **highScores;
	int **combos;
	float **accuracy;
};

char **filesCaching = 0;

#ifdef __unix
void mapInfoLoading(struct mapInfoLoadingArgs *args)
#else
DWORD WINAPI *mapInfoLoading(struct mapInfoLoadingArgs *args)
#endif
{
	lockLoadingMutex();
	_loading++;
	unlockLoadingMutex();
	static int oldAmount = 0;
	FilePathList files = LoadDirectoryFiles("maps/");

	int amount = 0;
	for(int i = 0; i < files.count; i++)
	{
		char str [100];
		snprintf(str, 100, "%s/map.data", files.paths[i]);
		if(FileExists(str))
		{
			amount++;
		}
	}
	
	char **newFiles = malloc(sizeof(char *) * amount);
	for(int i = 0, j = 0; i < files.count && j < amount; i++)
	{
		char str [100];
		snprintf(str, 100, "%s/map.data", files.paths[i]);
		if(FileExists(str))
		{
			newFiles[j] = malloc(sizeof(char) * (strlen(files.paths[i])+1));
			strcpy(newFiles[j], files.paths[i]);
			j++;
		}
	}
	UnloadDirectoryFiles(files);

	if (args->highScores != 0)
	{
		*args->highScores = realloc(*args->highScores, amount * sizeof(int));
		*args->combos = realloc(*args->combos, amount * sizeof(int));
		*args->accuracy = realloc(*args->accuracy, amount * sizeof(float));
		char **tmp = calloc(amount, sizeof(char *));
		for (int i = 0; i < amount && i < oldAmount; i++)
		{
			tmp[i] = filesCaching[i];
		}
		for (int i = oldAmount; i < amount; i++)
			tmp[i] = calloc(100, sizeof(char));
		for (int i = amount; i < oldAmount; i++)
			free(filesCaching[i]);
		free(filesCaching);
		filesCaching = tmp;

		Map *tmp2 = calloc(amount, sizeof(Map));
		for (int i = 0; i < amount && i < oldAmount; i++)
		{
			tmp2[i] = _paMaps[i];
		}
		for (int i = amount; i < oldAmount; i++)
			freeMap(&_paMaps[i]);
		free(_paMaps);
		_paMaps = tmp2;
	}
	else
	{
		*args->highScores = malloc(amount * sizeof(int));
		*args->combos = malloc(amount * sizeof(int));
		*args->accuracy = malloc(amount * sizeof(float));
		filesCaching = calloc(amount, sizeof(char *));
		for (int i = 0; i < amount; i++)
			filesCaching[i] = calloc(100, sizeof(char));
		_paMaps = calloc(amount, sizeof(Map));
	}
	oldAmount = amount;
	int mapIndex = 0;
	for (int i = 0; i < amount; i++)
	{
		if (newFiles[i][0] == '.')
			continue;
		// check for cache
		bool cacheHit = false;
		for (int j = 0; j < amount; j++)
		{
			if (!filesCaching[j][0])
				continue;
			if (strncmp(filesCaching[j], newFiles[i], 100) == 0)
			{
				// cache hit
				printf("cache hit! %s\n", newFiles[i]);
				cacheHit = true;
				if (mapIndex == j)
				{
					break;
				}
				strcpy(filesCaching[mapIndex], filesCaching[j]);
				_paMaps[mapIndex] = _paMaps[j];
				_paMaps[j] = (Map){0};
				readScore(&_paMaps[mapIndex],
						  &((*args->highScores)[mapIndex]),
						  &((*args->combos)[mapIndex]),
						  &((*args->accuracy)[mapIndex]));
				break;
			}
		}
		if (cacheHit)
		{
			mapIndex++;
			continue;
		}
		printf("cache miss %s %i\n", newFiles[i], mapIndex);
		// cache miss

		freeMap(&_paMaps[mapIndex]);
		_paMaps[mapIndex] = loadMapInfo(newFiles[i]);
		if(_paMaps[mapIndex].name == 0)
		{
			printf("skipping map, failed to load\n");
			continue;
		}
		if (_paMaps[mapIndex].name != 0)
		{
			readScore(&_paMaps[mapIndex],
					  &((*args->highScores)[mapIndex]),
					  &((*args->combos)[mapIndex]),
					  &((*args->accuracy)[mapIndex]));
		}

		// caching
		strcpy(filesCaching[mapIndex], newFiles[i]);

		mapIndex++;
	}
	lockLoadingMutex();
	_loading--;
	unlockLoadingMutex();
	*args->amount = mapIndex;
	free(args);
}

#ifdef __unix
void loadMapImage(Map *map)
#else
DWORD WINAPI *loadMapImage(Map * map)
#endif
{
	printf("loading map image %s\n", map->imageFile);

	//disabled loading lock because some images refuse to load
	// lockLoadingMutex();
	// _loading++;
	// unlockLoadingMutex();
	Image img ={0};
	char str [100];
	snprintf(str, 100, "%s/%s", map->folder, map->imageFile);
	img = LoadImage(str);
	map->cpuImage = img;
	if(img.width==0) //failed to load so setting it back to -1
		map->cpuImage.width=-2;
	// lockLoadingMutex();
	// _loading--;
	// unlockLoadingMutex();
}

#include <unistd.h>

void fMapSelect(bool reset)
{
	static int amount = 0;
	static int *highScores;
	static int *combos;
	static float *accuracy;
	static int selectedMap = -1;
	static float selectMapTransition = 1;
	static int hoverMap = -1;
	static float hoverPeriod = 0;
	static bool selectingMods = false;
	_musicSpeed = 1;
	static char search[100];
	static bool searchSelected;

	if (selectMapTransition < 1)
		selectMapTransition += GetFrameTime() * 10;
	if (selectMapTransition > 1)
		selectMapTransition = 1;
	checkFileDropped();
	if (_mapRefresh)
	{
		struct mapInfoLoadingArgs *args = malloc(sizeof(struct mapInfoLoadingArgs));
		args->amount = &amount;
		args->combos = &combos;
		args->highScores = &highScores;
		args->accuracy = &accuracy;
		createThread((void *(*)(void *))mapInfoLoading, args);
		_mapRefresh = false;
	}
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	if (selectingMods)
	{
		_playMenuMusic = true;
		_musicPlaying = false;
		hoverPeriod = 0;
		drawVignette();
		if (IsKeyPressed(KEY_ESCAPE) || interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
		{
			selectingMods = false;
			return;
		}
		// draw all modifiers
		int amountMods = sizeof(_mods) / sizeof(_mods[0]);
		for (int i = 0; i < amountMods; i++)
		{
			int x = i % 3;
			int y = i / 3;
			if (interactableButton(_mods[i].name, 0.03, getWidth() * (0.2 + x * 0.22), getHeight() * (0.6 + y * 0.12), getWidth() * 0.2, getHeight() * 0.07))
			{
				// enable mod
				int modId = _mods[i].id;
				bool foundDouble = false;
				for (int j = 0; j < amountMods; j++)
				{
					// find doubles
					if (_activeMod[j] != NULL && modId == _activeMod[j]->id)
					{
						foundDouble = true;
						// disable mod
						_activeMod[j] = 0;
						break;
					}
				}
				if (!foundDouble)
				{
					// add new mod
					// find empty spot
					for (int j = 0; j < 100; j++)
					{
						if (_activeMod[j] == 0)
						{
							_activeMod[j] = &(_mods[i]);
							break;
						}
					}
				}
			}
		}

		// print selected / active mods
		int modsSoFar = 0;
		for (int i = 0; i < 100; i++)
		{
			if (_activeMod[i] != 0)
			{
				drawText(_activeMod[i]->name, getWidth() * (0.05 + 0.12 * modsSoFar), getHeight() * 0.9, getWidth() * 0.03, WHITE);
				modsSoFar++;
			}
		}

		drawCursor();
		return;
	}

	drawCSS("theme/mapSelect.css");

	// DrawRectangle(0, 0, getWidth(), getHeight()*0.13, BLACK);
	static float menuScroll = 0;
	static float menuScrollSmooth = 0;
	menuScroll += GetMouseWheelMove() * .04;
	menuScrollSmooth += (menuScroll - menuScrollSmooth) * GetFrameTime() * 15;
	if (IsMouseButtonDown(0))
	{ // scroll by dragging
		menuScroll += GetMouseDelta().y / getHeight();
	}
	const float scrollSpeed = .03;
	if (IsKeyDown(KEY_UP))
	{
		menuScroll += scrollSpeed;
	}
	if (IsKeyDown(KEY_DOWN))
	{
		menuScroll -= scrollSpeed;
	}
	menuScroll = clamp(menuScroll, -.2 * floor(amount / 2), 0);

	if(UIBUttonPressed("modsButton"))
	// if (interactableButton("Mods", 0.03, getWidth() * 0.2, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		selectingMods = true;
	}

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	//interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
	}

	UITextBox(search, "searchBox");

	CSS_Object * searchText = getCSS_ObjectPointer("searchText");
	CSS_Object * searchBox = getCSS_ObjectPointer("searchBox");

	if(searchText && searchBox)
		searchText->active = search[0] == '\0' && !searchBox->selected &&
			!mouseInRect((Rectangle){.x=searchBox->x*getWidth(), .y=searchBox->y*getHeight(), .width=searchBox->width*getWidth(), .height=searchBox->height*getHeight()});

	if (hoverMap == -1)
	{
		_musicFrameCount = 1;
		hoverPeriod = 0;
	}
	else
	{
		_disableLoadingScreen = true;
		hoverPeriod += GetFrameTime();
		// if(_musicLength)
		// 	if (!*_musicLength)
		// 		_musicFrameCount = _paMaps[hoverMap].musicPreviewOffset * 48000 * 2;
	}
	// draw map button
	Rectangle mapSelectRect = (Rectangle){.x = 0, .y = getHeight() * 0.13, .width = getWidth(), .height = getHeight()};
	startScissor(mapSelectRect.x, mapSelectRect.y, mapSelectRect.width, mapSelectRect.height);
	int mapCount = -1;
	for (int i = 0; i < amount; i++)
	{
		// draw maps
		if (search[0] != '\0')
		{
			bool missingLetter = false;
			char str[100];
			snprintf(str, 100, "%s - %s", _paMaps[i].name, _paMaps[i].artist);
			int stringLength = strlen(str);
			for (int j = 0; j < 100 && search[j] != '\0'; j++)
			{
				bool foundOne = false;
				for (int k = 0; k < stringLength; k++)
				{
					if (tolower(search[j]) == tolower(str[k]))
						foundOne = true;
				}
				if (!foundOne)
				{
					missingLetter = true;
					break;
				}
			}
			if (missingLetter)
			{
				if(hoverMap == i)
					hoverMap = -1;
				continue;
			}
		}

		mapCount++;
		int x = getWidth() * 0.02 + getWidth() * 0.32 * (mapCount % 3);
		Rectangle mapButton = (Rectangle){.x = x, .y = menuScrollSmooth * getHeight() + getHeight() * ((floor(i / 3) > floor(selectedMap / 3) && selectedMap != -1 ? 0.3 : 0.225) + 0.3375 * floor(mapCount / 3)), .width = getWidth() * 0.3, .height = getHeight() * 0.3};
		
		if(mapButton.y + mapButton.height < 0 || mapButton.y > getHeight())
			continue;

				setCSS_Variable("mapname", _paMaps[i].name);
		setCSS_Variable("artist", _paMaps[i].artist);
		setCSS_VariableInt("difficulty", _paMaps[i].difficulty);

		CSS_Variable * nameAndArtistVar = getCSS_Variable("mapname_artist");
		if(nameAndArtistVar)
			snprintf(nameAndArtistVar->value, 100, "%s - %s", _paMaps[i].name, _paMaps[i].artist);


		CSS_Variable * musicLength = getCSS_Variable("musicLength");
		if(musicLength)
			snprintf(musicLength->value, 100, "%i:%i", _paMaps[i].musicLength/60, _paMaps[i].musicLength%60);

		
		//todo: lord forgive me for what i've done
		CSS_Object * mapImageObject = getCSS_ObjectPointer("mapImage");
		if(mapImageObject)
		{
			mapImageObject->image = _paMaps[i].image;
		}

		drawContainer("mapContainer", mapButton.x, mapButton.y);

		if(highScores[i] != 0)
		{
			CSS_Object * highScoreObject = getCSS_ObjectPointer("highscoreContainer");

			if(highScoreObject)
			{
				setCSS_VariableInt("highscore", highScores[i]);
				setCSS_VariableInt("combo", combos[i]);
				setCSS_VariableInt("accuracy", accuracy[i]);

				drawContainer("highscoreContainer", mapButton.x, mapButton.y);

				CSS_Object * rankObj = getCSS_ObjectPointer("rank");
				if(rankObj)
					drawRank(rankObj->x*getWidth()+mapButton.x, rankObj->y*getHeight()+mapButton.y, rankObj->width*getWidth(), rankObj->height * getWidth(), accuracy[i]);
			}
		}
		

		if ((mouseInRect(mapButton) || selectedMap == i) && mouseInRect(mapSelectRect))
		{
			if (hoverPeriod > 1 && hoverPeriod < 2)
			{
				// play music
				char str[100];
				snprintf(str, 100, "%s/%s", _paMaps[i].folder, _paMaps[i].musicFile);
				loadMusic(&_paMaps[i].music, str, _paMaps[i].musicPreviewOffset);
				_playMenuMusic = false;
				_musicFrameCount = _paMaps[i].musicPreviewOffset * 48000 * 2;
				_musicPlaying = true;
				hoverPeriod++;
			}
			hoverMap = i;
			_disableLoadingScreen = true;
		}
		else if ((!mouseInRect(mapButton) || !mouseInRect(mapSelectRect)) && hoverMap == i)
		{
			hoverMap = -1;
			_playMenuMusic = true;
			_musicPlaying = false;
			hoverPeriod = 0;
			_musicFrameCount = 1;
		}

		if(_paMaps[i].cpuImage.width == 0)
		{
			_paMaps[i].cpuImage.width = -1;
			createThread((void *(*)(void *))loadMapImage, &_paMaps[i]);
			_paMaps[i].cpuImage.width = -1;
		}

		if (selectedMap == i)
		{
			if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
				selectedMap = -1;

			Rectangle buttons = (Rectangle){.x = mapButton.x, .y = mapButton.y + mapButton.height, .width = mapButton.width, .height = mapButton.height * 0.15 * selectMapTransition};
			
			drawContainer("mapButtonsContainer", mapButton.x, mapButton.y);

			bool playButton = UIBUttonPressed("playButton");
			bool editorButton = UIBUttonPressed("editorButton");
			bool exportButton = UIBUttonPressed("exportButton");

			if(!mouseInRect(mapSelectRect))
			{
				playButton = false;
				editorButton = false;
				exportButton = false;
			}
			
			if (playButton || editorButton || exportButton)
			{
				_map = &_paMaps[i];
				setMusicStart();
				_musicHead = 0;
				printf("selected map!\n");
				_transition = 0.1;
				_disableLoadingScreen = false;
				_musicPlaying = false;

				CSS_Object * mapImageObject = getCSS_ObjectPointer("mapImage");
				if(mapImageObject)
				{
					mapImageObject->image = (Texture2D){0};
				}

				loadMap();

				//wait until map image is loaded
				// if(_pGameplayFunction != &fMapSelect)
				// {
				// 	float startTime = (float)clock()/CLOCKS_PER_SEC;
				// 	for(int i = 0; i < 20 && _map->cpuImage.width < 1; i++)
				// 	{
				// 		// #ifdef _WIN32
				// 		// 	Sleep(100);
				// 		// #else
				// 		// 	usleep(100);
				// 		// #endif
				// 	}

				// 	if(_map->cpuImage.width < 1)
				// 		_map->image = _background;
				// }
			}
			
			if (playButton)
			{
				_pNextGameplayFunction = &fPlaying;
				_pGameplayFunction = &fCountDown;
				fCountDown(true);
				fPlaying(true);
			}
			if (editorButton)
			{
				_pNextGameplayFunction = &fEditor;
				_pGameplayFunction = &fEditor;
				fEditor(true);
			}
			if (exportButton)
			{
				_pGameplayFunction = &fExport;
			}
			DrawRectangleGradientV(mapButton.x, mapButton.y + mapButton.height, mapButton.width, mapButton.height * 0.05 * selectMapTransition, ColorAlpha(BLACK, 0.3), ColorAlpha(BLACK, 0));
		}
		else
		{
			// drawMapThumbnail(mapButton, &_paMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], false);

			if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
			{
				playAudioEffect(_buttonSe);
				selectedMap = i;
				selectMapTransition = 0;
				hoverPeriod = 0;
				_musicFrameCount = 1;
			}

			if(_paMaps[i].image.id == 0)
			{
				//load map image onto gpu(cant be loaded in sperate thread because opengl >:( )
				if(_paMaps[i].cpuImage.width > 0)
				{
					_paMaps[i].image = LoadTextureFromImage(_paMaps[i].cpuImage);
					UnloadImage(_paMaps[i].cpuImage);
					_paMaps[i].cpuImage.width = -1;
					if(_paMaps[i].image.id == 0)
						_paMaps[i].image.id = -1; 
					else
						SetTextureFilter(_paMaps[i].image, TEXTURE_FILTER_BILINEAR);
				}
			}
		}
	}

	CSS_Object * mapImageObject = getCSS_ObjectPointer("mapImage");
	if(mapImageObject)
	{
		mapImageObject->image = (Texture2D){0};
	}
	
	drawVignette();

	endScissor();

	DrawRectangleGradientV(0, getHeight()*0.8, getWidth(), getHeight()*0.21, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.8));


	if (hoverMap != -1 || selectedMap != -1)
	{
		int selMap = selectedMap != -1 ? selectedMap : hoverMap;
		if(_paMaps[selMap].name != 0)
		{
			char str[100];
			strcpy(str, _paMaps[selMap].name);
			strcat(str, " - ");
			strcat(str, _paMaps[selMap].artist);
			int textSize = measureText(str, getWidth() * 0.05);
			drawText(str, getWidth() * 0.9 - textSize, getHeight() * 0.92, getWidth() * 0.05, WHITE);
		}
	}

	drawCursor();
}



void fExport(bool reset)
{
	_pGameplayFunction = &fMainMenu;
	makeMapZip(_map);
	char str[300];
	strcpy(str, GetWorkingDirectory());
	strcat(str, "/");
	strcat(str, _map->name);
	strcat(str, ".zip");
	SetClipboardText(str);
	resetBackGround();
	strcpy(_notfication, "exported map");
}

void fNewMap(bool reset)
{
	static Map newMap = {0};
	static void *pMusic = 0;
	static char pMusicExt[50];
	static int pMusicSize = 0;
	static void *pImage = 0;
	static int imageSize = 0;

	static Audio previewAudio;
	static Texture2D previewTexture;

	if (reset)
	{
		if(!newMap.name)
			newMap.name = malloc(100);
		strcpy(newMap.name, "name");
		
		if(!newMap.artist)
			newMap.artist = malloc(100);
		strcpy(newMap.artist, "Artist");

		if(!newMap.mapCreator)
			newMap.mapCreator = malloc(100);
		strcpy(newMap.mapCreator, _playerName);

		if(!newMap.folder)
			newMap.folder = malloc(100);
		newMap.folder[0] = '\0';
		newMap.bpm = 100;
		newMap.beats = 4;
	}
	
	ClearBackground(BLACK);
	DrawTextureTiled(_background, (Rectangle){.x = GetTime() * 50, .y = GetTime() * 50, .height = _background.height, .width = _background.width},
					 (Rectangle){.x = 0, .y = 0, .height = getHeight(), .width = getWidth()}, (Vector2){.x = 0, .y = 0}, 0, 0.2, WHITE);

	drawVignette();

	drawCSS("theme/newMap.css");

	int middle = getWidth() / 2;

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	{
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
		return;
	}

	if (UIBUttonPressed("finishButton"))
	{
		if (pMusic == 0)
			return;
		if (pImage == 0)
			return;

		UnloadTexture(previewTexture);

		if(previewAudio.size)
			free(previewAudio.data);

		makeMap(&newMap);
		char str[100];
		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/song");
		strcat(str, pMusicExt);
		FILE *file = fopen(str, "wb");
		fwrite(pMusic, pMusicSize, 1, file);
		fclose(file);

		newMap.musicFile = malloc(100);
		snprintf(newMap.musicFile, 100, "/song%s", pMusicExt);

		strcpy(str, "maps/");
		strcat(str, newMap.name);
		strcat(str, "/image.png");
		file = fopen(str, "wb");
		fwrite(pImage, imageSize, 1, file);
		fclose(file);
		if (newMap.bpm == 0)
			newMap.bpm = 1;
		_pNextGameplayFunction = &fEditor;
		_amountNotes = 0;
		_noteIndex = 0;
		_pGameplayFunction = &fEditor;
		_transition = 0.1;
		newMap.folder = malloc(100);
		snprintf(newMap.folder, 100, "maps/%s/", newMap.name);
		_map = &newMap;
		freeNotes();
		saveMap();
		printf("map : %s\n", newMap.name);
		loadMap();
		_noBackground = 0;
		setMusicStart();
		_musicHead = 0;
		_transition = 0.1;
		_disableLoadingScreen = false;
		// startMusic();
		return;
	}

	UITextBox(newMap.name, "nameBox");
	UITextBox(newMap.artist, "artistBox");
	UITextBox(newMap.mapCreator, "mapCreatorBox");

	newMap.difficulty = UIValueInteractable(newMap.difficulty, "difficultyBox");
	newMap.bpm = UIValueInteractable(newMap.bpm, "bpmBox");

	int textSize = measureText("missing music file", getWidth() * 0.03);
	if (pMusic == 0)
		drawText("missing music file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.6, getWidth() * 0.03, WHITE);
	else{
		if(interactableButton("preview", 0.03, getWidth() * 0.1, getHeight()*0.6, getWidth()*0.15, getHeight()*0.05))
			playAudioEffect(previewAudio);
	}

	textSize = measureText("missing image file", getWidth() * 0.03);
	if (pImage == 0)
		drawText("missing image file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.7, getWidth() * 0.03, WHITE);
	else
		DrawTextureEx(previewTexture, (Vector2){.x=getWidth()*0.1, .y=getHeight()*0.7}, 0, getWidth()*0.15 / previewTexture.width, WHITE);
	// drawText("got image file", getWidth() * 0.2 - textSize / 2, getHeight() * 0.7, getWidth() * 0.03, WHITE);

	drawCursor();

	// file dropping
	if (IsFileDropped() || ((IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))))
	{
		int amount = 0;
		char **files;
		FilePathList filePaths;
		bool keyOrDrop = true;
		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			// copy paste
			char *str = (char *) GetClipboardText();

			int file = 0;
			int partIndex = 0;
			files = malloc(strlen(str));
			char * part = calloc( strlen(str), sizeof(char));
			for (int i = 0; str[i] != '\0'; i++)
			{
				part[partIndex++] = str[i];
				if (str[i + 1] == '\n' || str[i + 1] == '\0')
				{
					i++;
					part[partIndex + 1] = '\0';
					printf("file %i: %s\n", file, part);
					if (FileExists(part))
					{
						printf("\tfile exists\n");
						files[file] = malloc(partIndex);
						strcpy(files[file], part);
						file++;
					}
					partIndex = 0;
				}
			}
			amount = file;
			keyOrDrop = false;
		}
		else
		{
			filePaths = LoadDroppedFiles();
			files = filePaths.paths;
			amount = filePaths.count;
		}

		for (int i = 0; i < amount; i++)
		{
			const char *ext = GetFileExtension(files[i]);
			if (strcmp(ext, ".png") == 0)
			{
				if (previewTexture.id != 0)
					UnloadTexture(previewTexture);
				previewTexture = LoadTexture(files[i]);

				if (pImage != 0)
					free(pImage);
				
				FILE *file = fopen(files[i], "rb");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pImage = malloc(size);
				fread(pImage, size, 1, file);
				fclose(file);
				imageSize = size;
			}

			if (strcmp(ext, ".mp3") == 0)
			{
				if (pMusic != 0)
					free(pMusic);
				FILE *file = fopen(files[i], "rb");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pMusic = malloc(size);
				fread(pMusic, size, 1, file);
				fclose(file);
				strcpy(pMusicExt, ext);
				pMusicSize = size;

				loadAudio(&previewAudio, files[i]);
			}

			if (strcmp(ext, ".wav") == 0)
			{
				if (pMusic != 0)
					free(pMusic);
				FILE *file = fopen(files[i], "rb");
				fseek(file, 0L, SEEK_END);
				int size = ftell(file);
				rewind(file);
				pMusic = malloc(size);
				fread(pMusic, size, 1, file);
				strcpy(pMusicExt, ext);
				fclose(file);
				pMusicSize = size;
			}
		}
		if (!keyOrDrop)
		{
			for (int i = 0; i < amount; i++)
				free(files[i]);
			free(files);
		}
		else
			UnloadDroppedFiles(filePaths);
	}
}

void fIntro(bool reset)
{
	static float time = 0;
	fMainMenu(true);
	DrawRectangle(0, 0, getWidth(), getHeight(), ColorAlpha(BLACK, (1 - time) * 0.7));
	DrawRing((Vector2){.x = getWidth() / 2, .y = getHeight() / 2}, time * getWidth() * 1, time * getWidth() * 0.8, 0, 360, 360, ColorAlpha(WHITE, 1 - time));
	time += fmin(GetFrameTime() / 2, 0.016);
	if (time > 1)
	{
		_pGameplayFunction = &fMainMenu;
	}
}

bool mouseInRect(Rectangle rect)
{
	int x = GetMouseX();
	int y = GetMouseY();
	if (rect.x < x && rect.y < y && rect.x + rect.width > x && rect.y + rect.height > y)
	{
		return true;
	}
	return false;
}

void textBox(Rectangle rect, char *str, bool *selected)
{
	static int cursor = 0;
	static double blinkingTimeout = 0;

	if(!str)
		return;
	
	float fontSize = 0.03;
	int screenSize = getWidth() > getHeight() ? getHeight() : getWidth();

	drawButton(rect, str, fontSize);
	fontSize *= 1.3;

	if(mouseInRect(rect) && IsMouseButtonReleased(0))
	{
		blinkingTimeout = GetTime();
		*selected = true;
		//calculate cursor pos
		int fullLength = measureText(str, fontSize*screenSize);
		int x = rect.x + rect.width / 2 - fullLength / 2;
		float textPos = ((GetMouseX()-x) / (float)fullLength);
		cursor = textPos * strlen(str) + 1;

		if(cursor < 0)
			cursor = 0;

		if(cursor > strlen(str))
			cursor = strlen(str);

	}

	if (*selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while (c != 0)
		{
			char strPart1[100];
			strcpy(strPart1, str);
			strPart1[cursor] = '\0';

			char strPart2[100];
			strcpy(strPart2, str+cursor);

			snprintf(str, 100, "%s%c%s\0",strPart1, c, strPart2);
			str[strlen(str)+2] = '\0';

			cursor++;
			c = GetCharPressed();
			blinkingTimeout = GetTime();
		}

		if(IsKeyPressed(KEY_LEFT))
		{
			cursor--;
			if(cursor < 0)
				cursor = 0;
			blinkingTimeout = GetTime();
		}

		if(IsKeyPressed(KEY_RIGHT))
		{
			cursor++;
			if(cursor > strlen(str))
				cursor = strlen(str);
			blinkingTimeout = GetTime();
		}

		if(IsKeyPressed(KEY_LEFT) && IsKeyDown(KEY_LEFT_CONTROL))
		{
			cursor = 0;
			blinkingTimeout = GetTime();
		}

		if(IsKeyPressed(KEY_RIGHT) && IsKeyDown(KEY_LEFT_CONTROL))
		{
			cursor = strlen(str);
			blinkingTimeout = GetTime();
		}

		if (IsKeyPressed(KEY_BACKSPACE) && cursor != 0)
		{
			int prev = cursor;
			cursor--;
			if(cursor < 0)
				cursor = 0;
			
			char strCopy[100];
			strcpy(strCopy, str);
			strCopy[cursor] = '\0';

			snprintf(str, 100, "%s%s",strCopy, strCopy+prev);
			blinkingTimeout = GetTime();
		}

		if (IsKeyPressed(KEY_DELETE) && cursor != strlen(str))
		{
			char strCopy[100];
			strcpy(strCopy, str);
			strCopy[cursor] = '\0';

			char strCopy2[100];
			strcpy(strCopy2, str+cursor+1);

			snprintf(str, 100, "%s%s",strCopy, strCopy2);
			blinkingTimeout = GetTime();
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_BACKSPACE))
		{
			char strCopy[100];
			strcpy(strCopy, str+cursor);
			strcpy(str, strCopy);
			cursor = 0;
			blinkingTimeout = GetTime();
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_DELETE))
		{
			str[cursor] = '\0';
			blinkingTimeout = GetTime();
		}

		if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_ENTER))
		{
			*selected = false;
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			char * clipboard = (char*) GetClipboardText();

			if(!clipboard)
				return;
			
			char tmpStr[100];
			strcpy(tmpStr, str);
			tmpStr[cursor] = '\0';

			char ogString[100];
			strcpy(ogString, str+cursor);

			snprintf(str, 100, "%s%s%s", tmpStr, clipboard, ogString);

			// free(clipboard);
		}
	}

	if (*selected)
	{
		//draw cursor
		char tmpStr[100];
		strcpy(tmpStr, str);
		tmpStr[cursor] = '\0';
		int cursorLength = measureText(tmpStr, fontSize*screenSize);

		int fullLength = measureText(str, fontSize*screenSize);
		int x = rect.x + rect.width / 2 - fullLength / 2;
		if((int)(GetTime()*2) % 2 == 0 || GetTime() - blinkingTimeout < 0.5)
			DrawRectangle(x+cursorLength, rect.y, getWidth()*0.003, rect.height, DARKGRAY);
		// DrawRectangle(rect.x + rect.width * 0.2, rect.y + rect.height * 0.75, rect.width * 0.6, rect.height * 0.1, DARKGRAY);
	}

	if (*selected && !mouseInRect(rect) && IsMouseButtonReleased(0))
	{
		*selected = false;
		cursor = 0;
	}
}

void numberBox(Rectangle rect, int *number, bool *selected)
{
	if(!number && !selected)
		return;
	
	char str[10];
	snprintf(str, 10, "%i", *number);
	drawButton(rect, str, 0.03);
	if ((mouseInRect(rect) && IsMouseButtonPressed(0)) || *selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while (c != 0)
		{
			if(c >= '0' && c <= '9')
			{
				*number *= 10;
				*number += c - '0';
			}
			c = GetCharPressed();
		}

		if(IsKeyPressed(KEY_BACKSPACE))
		{
			*number -= (*number)%10;
			*number /= 10;
		}

		if(IsKeyPressed(KEY_MINUS))
		{
			*number *= -1;
		}
	}
	if (*selected)
		DrawRectangle(rect.x + rect.width * 0.2, rect.y + rect.height * 0.75, rect.width * 0.6, rect.height * 0.1, DARKGRAY);

	if ((!mouseInRect(rect) && IsMouseButtonPressed(0)) || IsKeyPressed(KEY_ENTER))
		*selected = false;
}

void slider(Rectangle rect, bool *selected, int *value, int max, int min)
{

	Color color = WHITE;
	if (*selected)
		color = LIGHTGRAY;

	float sliderMapped = (*value - min) / (float)(max - min);
	DrawRectangle(rect.x, rect.y, rect.width, rect.height, WHITE);
	DrawCircle(rect.x + rect.width * sliderMapped, rect.y + rect.height * 0.5, rect.height, color);

	

	Rectangle extendedRect = (Rectangle){.x=rect.x-rect.height, .y=rect.y-rect.height/2, .width=rect.width+rect.height*2, .height=rect.height+rect.height};
	if ((mouseInRect(extendedRect) && IsMouseButtonPressed(0)) || (*selected && IsMouseButtonDown(0)))
	{
		*selected = true;
		*value = ((GetMouseX() - rect.x) / rect.width) * (max - min) + min;
		
		if (*value > max)
			*value = max;
		if (*value < min)
			*value = min;

		//draw number next to mouse
		char str[10];
		snprintf(str, 10, "%i", *value);
		int length = measureText(str, getWidth()*0.035);
		DrawRectangle(GetMouseX()+getWidth()*0.035, GetMouseY(), length+ getWidth()*0.01, getWidth()*0.035, ColorAlpha(BLACK, 0.8));
		drawText(str, GetMouseX()+getWidth()*0.04, GetMouseY(), getWidth()*0.035, WHITE);

	}

	if (!IsMouseButtonDown(0))
		*selected = false;


}

bool interactableButton(char *text, float fontScale, float x, float y, float width, float height)
{
	Rectangle button = (Rectangle){.x = x, .y = y, .width = width, .height = height};
	drawButton(button, text, fontScale);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		playAudioEffect(_buttonSe);
		return true;
	}
	return false;
}

bool interactableButtonNoSprite(char *text, float fontScale, float x, float y, float width, float height)
{
	Rectangle button = (Rectangle){.x = x, .y = y, .width = width, .height = height};
	drawButtonNoSprite(button, text, fontScale);

	if (IsMouseButtonReleased(0) && mouseInRect(button))
	{
		playAudioEffect(_buttonSe);
		return true;
	}
	return false;
}