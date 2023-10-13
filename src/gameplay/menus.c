

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
#include "../commands.h"

CSS * _pCSS = 0;

bool _mapRefresh = true;
bool _anyUIButtonPressed = false;

#define freeArray(arr) \
	if(arr) { \
		free(arr); \
		arr = 0; \
	}

void freeCSS_Image(CSS_Image * img)
{
	if(img->file)
		printf("image file : %i\n", img->file);
	freeArray(img->file);

	if(img->isCopy)
		return;
	
	if(img->state == CSSImage_loaded)
	{
		UnloadTexture(img->texture);
	}
	if(img->state == CSSImage_loading && img->stop != 0)
	{
		*img->stop = true; //sends signal to loading function to not write to address
	}
}

void freeCSS_Object(CSS_Object * object)
{
	if(!object)
		return;

	freeCSS_Image(&object->image);

	if(object->name)
		free(object->name);

	if(object->parent)
		free(object->parent);
	
	if(object->text && !object->usesVariable)
		free(object->text);

	if(object->hintText && !object->usesVariableHint)
		free(object->hintText);

	if(object->loadFile)
		free(object->loadFile);

	if(object->makeActive)
		free(object->makeActive);

	if(object->children)
		free(object->children);

	if(object->command)
		free(object->command);
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

int _drawTick = 0;

void drawCSS_Object(CSS_Object * object)
{
	if(!object->active)
	{
		object->selected = false;
		return;
	}

	if(object->drawTick == _drawTick)
		return;
	
	object->drawTick = _drawTick;

	float scrollValue = 0; //objects cannot scroll themselves
	
	float width = getWidth();
	float height = getHeight();
	if(object->keepAspectRatio)
		width = height;

	Rectangle rect = (Rectangle){.x=object->x*getWidth(), .y=(object->y)*getHeight(), .width=object->width*width, .height=object->height*height};


	// printf("object: %i\n", object-_pCSS->objects);
	int scissors = 0;
	if(object->parentObj != -1)
	{
		CSS_Object parent = *object;

		while(parent.parentObj != -1)
		{
			// printf("parent: %i\n", parent.parentObj);
			parent = _pCSS->objects[parent.parentObj];

			if(!parent.active)
			{
				for(;scissors>0;scissors--)
				{
					endScissor();
				}

				return;
			}

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
				scissors++;
			}

			if(parent.scrollable)
			{
				scrollValue += parent.scrollValue;
			}
		}
	}

	rect.y += scrollValue * getHeight();

	if(mouseInRect(rect) || object->selected)
	{
		object->hoverTime = fmin(GetFrameTime()+object->hoverTime, 0.1);
	}else
		object->hoverTime = fmax(object->hoverTime-GetFrameTime(), 0);

	float growAmount = object->growOnHover * fmin(object->hoverTime*15, 1);

	if(mouseInRect(rect) && IsMouseButtonDown(0))
		growAmount *= -1;


	Rectangle rectInteract = rect;
	rect.x -= rect.width*growAmount/2;
	rect.y -= rect.height*growAmount/2;

	rect.width += rect.width*growAmount;
	rect.height += rect.height*growAmount;

	float fontSize = object->fontSize * (1+growAmount);

	float rotation = fmin(object->hoverTime*15, 1) * object->rotateOnHover;

	if(object->centered)
	{
		rect.x -= rect.width/2;
		rect.y -= rect.height/2;
	}

	bool skip = false;

	//if rect is out of bounds skip
	if(rect.x + rect.width < 0)
		skip = true;
	if(rect.x > getWidth())
		skip = true;

	if(rect.y + rect.height < 0)
		skip = true;
	if(rect.y > getHeight())
		skip = true;

	if(_scissorMode)
	{
		if(rect.x + rect.width < _scissors[_scissorIndex].x)
			skip = true;
		if(rect.x > _scissors[_scissorIndex].width+_scissors[_scissorIndex].x)
			skip = true;

		if(rect.y + rect.height < _scissors[_scissorIndex].y)
			skip = true;
		if(rect.y > _scissors[_scissorIndex].y + _scissors[_scissorIndex].height)
			skip = true;
	}

	if(skip)
	{
		for(;scissors>0; scissors--)
			endScissor();
		
		return;
	}

	
	if(mouseInRect(rectInteract) && IsMouseButtonPressed(0) && (!_scissorMode || mouseInRect(_scissors[_scissorIndex])))
		object->pressedOn = true;

	enum CSS_Type type = object->type;

	if(type == css_toggle || type == css_button || type == css_buttonNoSprite || type == css_slider || type == css_textbox || type == css_numberbox)
	{
		if(mouseInRect(rectInteract) && (IsMouseButtonPressed(0) || IsMouseButtonReleased(0)))
		{
			_anyUIButtonPressed = true;

		}
	}

	Texture2D image = {0};
	if(object->image.state == CSSImage_loaded)
	{
		image = object->image.texture;
	}

	if(object->image.file && object->image.state == CSSImage_notLoaded)
	{
		loadCSS_Image_Immediate(&object->image);
	}

	switch(object->type)
	{
		case css_text:
			if(object->text)
			{
				if (object->textClipping)
					drawTextInRect(rect, object->text, fontSize*getWidth(), object->color, mouseInRect(rectInteract));
				else
					drawText(object->text, rect.x, rect.y, fontSize*getWidth(), object->color);
			}
			// DrawRectangle(rect.x, rect.y, rect.width, rect.height, LIGHTGRAY);
			break;
		
		case css_image:
			if (object->image.state == CSSImage_rank)
			{
				drawRank(rect.x, rect.y, rect.width, rect.height, object->value);
			}else
				drawTextureCorrectAspectRatio(image, object->color, rect, rotation);
			break;

		case css_toggle:
			object->selected = false;

			if(mouseInRect(rectInteract) && IsMouseButtonReleased(0) && object->pressedOn)
			{
				object->value = !object->value;
				object->selected = true;
			}
			
			if(object->value)
			{
				drawButton(rect, "X", object->fontSize);
			}else
				drawButton(rect, "", 1);

			
			break;
		
		case css_rectangle:
			DrawRectangle(rect.x, rect.y, rect.width, rect.height, object->color);
			break;

		case css_button:
			if(image.id)
				drawButtonPro(rect, object->text, fontSize, image, rotation);
			else
				drawButton(rect, object->text, fontSize);

			object->selected = mouseInRect(rectInteract) && IsMouseButtonReleased(0) && object->pressedOn;
			break;

		case css_buttonNoSprite:
			if (object->opacity > 0)
				drawButtonNoSprite(rect, object->text, fontSize);
			object->selected = mouseInRect(rectInteract) && IsMouseButtonReleased(0) && object->pressedOn;
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
		
	}

	if(object->scrollable && mouseInRect(rectInteract))
	{
		float frametime = GetFrameTime();
		if(frametime > 1/MAXFPS)
			frametime = 1/MAXFPS;

		//limit scrolling when bad framerate (ie loading map images)
		object->scrollValue += GetMouseWheelMove() * frametime * 15;

		if(object->scrollValue > 0)
			object->scrollValue = 0;
	}

	if(object->hoverTime > 0.05 && object->hintText)
	{
		drawHint(rect, object->hintText);
	}

	for(int i = 0; i < object->childrenCount; i++)
	{
		drawCSS_Object(&_pCSS->objects[object->children[i]]);
	}

	for(;scissors > 0; scissors--)
	{
		endScissor();
	}

	if(object->type != css_toggle)
	{
		if(object->value > object->max)
			object->value = object->max;
		
		if(object->value < object->min)
			object->value = object->min;
	}

	if(IsMouseButtonReleased(0))
		object->pressedOn = false;
		

	if((object->type == css_button || object->type == css_buttonNoSprite) && object->selected)
	{
		//button action
		if(object->makeActive)
		{
			CSS_Object * activatedObject = &_pCSS->objects[object->makeActiveObj];
			activatedObject->active = !activatedObject->active;
		}

		if(object->command)
		{
			commandParser(object->command);
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
				char * file = malloc(100);
				strncpy(file, object->loadFile, 100);
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

	_drawTick++;

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
		char * file = malloc(100);
		strncpy(file, _pCSS->file, 100);
		freeCSS(_pCSS);
		free(_pCSS);
		loadCSS(file);
		free(file);

		if(!_pCSS)
			return;
	}

	_anyUIButtonPressed = false;

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

	int index = _pCSS->variableCount-1;

	_pCSS->variables[index].name = malloc(100);
	strncpy(_pCSS->variables[index].name, name, 100);
	_pCSS->variables[index].value = malloc(100);
	_pCSS->variables[index].value[0] = '\0';

	_pCSS->variables[index].nameCharBits = stringToBits(name);
	return _pCSS->variables[index].value;
}

CSS_Variable * getCSS_Variable(char * name)
{
	if(!name)
		return 0;
	
	int charBits = stringToBits(name);

	for(int i = 0; i < _pCSS->variableCount; i++)
	{
		if(charBits != _pCSS->variables[i].nameCharBits)
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
		strncpy(var->value, value, 100);
}

void setCSS_VariableInt(char * name, int value)
{
	if(!name)
		return;
	
	CSS_Variable * var = getCSS_Variable(name);
	if(var)
		snprintf(var->value, 100, "%i", value);
}

void loadCSS_Image_Immediate(CSS_Image * image)
{
	if(image->file == 0)
		return;
	
	image->texture = LoadTexture(image->file);
	image->state = CSSImage_loaded;
	image->stop = 0;

	GenTextureMipmaps(&image->texture);
	SetTextureFilter(image->texture, TEXTURE_FILTER_TRILINEAR);
}

void loadCSS_Image_OnViewing(CSS_Image * image)
{
	if(image->file == 0)
		return;
	
	image->state = CSSImage_notLoaded;
	image->stop = 0;
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
	_pCSS->file = malloc(100);
	strncpy(_pCSS->file, fileName, 100);
	_pCSS->objectCount = 0;
	_pCSS->objects = 0;
	_pCSS->variables = 0;
	_pCSS->variableCount = 0;

	bool mapRefresh = false;

	for(int i = 0; i < size; i++)
	{
		if(text[i] == '#')
		{
			i++;
			CSS_Object object = {0};
			object.opacity = 1;
			object.active = true;
			object.parentObj = -1;

			for(int nameIndex = i; nameIndex < size; nameIndex++)
			{
				if(text[nameIndex] == '{')
				{
					text[nameIndex] = '\0';
					object.name = malloc(100);
					strncpy(object.name, text+i, 100);
					i = nameIndex+1;
					break;
				}
			}

			object.nameCharBits = stringToBits(object.name);

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

							if(!strcmp(value, "toggle"))
								object.type = css_toggle;

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

							if(!strcmp(value, "array"))
								object.type = css_array;
							
							if(!strcmp(value, "array_element"))
								object.type = css_array_element;
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
									object.text = malloc(100);
									strncpy(object.text, _map->name, 100);
								}else if(!strcmp(value, "_playerName_"))
								{
									object.text = malloc(100);
									strncpy(object.text, _playerName, 100);
								}else if(!strcmp(value, "_notification_"))
								{
									object.text = malloc(100);
									strncpy(object.text, _notfication, 100);
								}else {
									object.text = malloc(100);
									strncpy(object.text, value, 100);
								}
							}
						}

						if(!strcmp(var, "hintText"))
						{
							if(strlen(value))
							{

								if(value[0] == '$')
								{
									object.hintText = addCSS_Variable(value+1);
									object.usesVariable = true;
								}else if(!strcmp(value, "_mapname_") && _map)
								{
									object.hintText = malloc(100);
									strncpy(object.hintText, _map->name, 100);
								}else if(!strcmp(value, "_playerName_"))
								{
									object.hintText = malloc(100);
									strncpy(object.hintText, _playerName, 100);
								}else if(!strcmp(value, "_notification_"))
								{
									object.hintText = malloc(100);
									strncpy(object.hintText, _notfication, 100);
								}else {
									object.hintText = malloc(100);
									strncpy(object.hintText, value, 100);
								}
							}
						}

						if(!strcmp(var, "parent"))
						{
							if(strlen(value))
							{
								object.parent = malloc(100);
								strncpy(object.parent, value, 100);
							}
						}

						if(!strcmp(var, "makeActive"))
						{
							if(strlen(value))
							{
								object.makeActive = malloc(100);
								strncpy(object.makeActive, value, 100);
							}
						}

						if(!strcmp(var, "command"))
						{
							if(strlen(value))
							{
								object.command = malloc(100);
								strncpy(object.command, value, 100);
								replaceTextVariables(object.command);
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
								object.loadFile = malloc(100);
								strncpy(object.loadFile, value, 100);
							}
						}


						if(!strcmp(var, "active"))
						{
							object.active = !strcmp(value, "yes");
						}

						if(!strcmp(var, "centered"))
						{
							object.centered = !strcmp(value, "yes");
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
							object.image = (CSS_Image){0};
							if(!strcmp(value, "_map_image_"))
							{
								if(_map)
								{
									object.image.file = malloc(100);
									sprintf(object.image.file, "%s/%s", _map->folder, _map->imageFile);
									loadCSS_Image_Immediate(&object.image);
								}else
								{
									object.image.file = malloc(100);
									strcpy(object.image.file, "_map_image_");
									object.image.state = CSSImage_notLoaded;
								}
							}else if(!strcmp(value, "_background_"))
							{
								object.image.state = CSSImage_loaded;
								object.image.isCopy = true;
								object.image.texture = _menuBackground;
							}else
							{
								object.image.file = malloc(100);
								strcpy(object.image.file, value);
								loadCSS_Image_Immediate(&object.image);
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

						if(!strcmp(var, "textClipping"))
						{
							object.textClipping = !strcmp(value, "yes");
						}

						if(!strcmp(var, "rotateOnHover"))
						{
							object.rotateOnHover = atof(value);
						}

						if(!strcmp(var, "font-size"))
						{
							object.fontSize = atof(value)/100.0;
						}

						if(!strcmp(var, "padding-left"))
						{
							object.paddingX = atof(value)/100.0;
						}

						if(!strcmp(var, "padding-top"))
						{
							object.paddingY = atof(value)/100.0;
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

			if(object.type == css_array && !strcmp(object.text, "maps"))
			{
				mapRefresh = true;
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

	if(mapRefresh)
	{
		mapInfoLoading(); //not multithreaded for now
	}

	//apply parents and children
	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		CSS_Object * obj = &_pCSS->objects[i];
		obj->parentObj = -1;
		if(obj->parent)
		{
			CSS_Object * parent = getCSS_ObjectPointer(obj->parent);
			obj->parentObj = parent-_pCSS->objects; //:P
			if(parent->children)
			{
				parent->childrenCount++;
				parent->children = realloc(parent->children, sizeof(int)*parent->childrenCount);
			}else
			{
				parent->childrenCount = 1;
				parent->children = malloc(sizeof(int)*parent->childrenCount);
			}
			parent->children[parent->childrenCount-1] = obj-_pCSS->objects; //:P
		}
	}

	//find makeActiveObj's based of makeActive's
	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		CSS_Object * obj = &_pCSS->objects[i];
		obj->makeActiveObj = 0; //first object is surely to exist

		if(obj->makeActive)
		{
			CSS_Object * chain = getCSS_ObjectPointer(obj->makeActive);

			if(chain > 0)
				obj->makeActiveObj = chain-_pCSS->objects; //:P
		}
	}

	//spawn elements of array
	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		CSS_Object obj = _pCSS->objects[i];
		
		if(obj.type != css_array)
			continue;

		//all conditions have been met, lets see how many times we have to spawn them

		if(!strcmp(obj.text, "maps"))
		{
			if(obj.active == false)
				continue;

			_pCSS->objects[i].arrayChildren = calloc(_mapsCount*obj.childrenCount, sizeof(CSS_Array_Child));
			_pCSS->objects[i].arrayChildCount = _mapsCount * obj.childrenCount;

			//spawn 1 per map
			for(int map = 0; map < _mapsCount; map++)
			{
				_map = &_paMaps[map];
				for(int child = 0; child < obj.childrenCount; child++)
				{
					CSS_Object * newChild = makeCSS_ObjectClone(_pCSS->objects[obj.children[child]]);
					
					int childIndex = newChild - _pCSS->objects;

					newChild->arrayChild = true;
					newChild->arrayObj = i; //points to parent / array object

					CSS_Array_Child * childArray = &_pCSS->objects[i].arrayChildren[map];

					childArray->enabled = newChild->active;
					childArray->index = childIndex;
					childArray->key = malloc(100);
					strcpy(childArray->key, _map->name);

				}
			}

			// printf("children count: %i", obj.childrenCount);

			_pCSS->objects[obj.children[0]].active = false;

			updateArrayChildren(&_pCSS->objects[i]);
		}
	}
}

void updateArrayChildren(CSS_Object * object)
{
	if(object->type != css_array)
		return;
	
	int width = object->width / (_pCSS->objects[object->children[0]].width+_pCSS->objects[object->children[0]].paddingX);
	float padding = object->width - width*(_pCSS->objects[object->children[0]].width+_pCSS->objects[object->children[0]].paddingX);
	padding *= 0.5;

	int activeChildCounter = 0;
	for(int i = 0; i < object->arrayChildCount; i++)
	{
		//todo: don't assume every child is the same width and height
		
		int x = activeChildCounter;
		int row = x / width;
		x -= row * width;
		CSS_Object * child = &_pCSS->objects[object->arrayChildren[i].index];
		child->x = x*(child->width+child->paddingX) + padding;
		float y = row * (child->height+child->paddingY);
		child->y = y;

		if(object->arrayChildren[i].enabled)
			activeChildCounter++;
		
		child->active = object->arrayChildren[i].enabled;
	}

	
}

#define stringcopy(str1, str2) \
	if(str2) { \
		str1 = malloc(100); \
		strcpy(str1, str2); \
	}

void replaceTextVariables(char * str)
{
	if(_map == 0)
		return;
	
	//todo: set check that it doesn't overgo it's bounds (maybe strncpy)
	char * newStr = malloc(100);
	newStr[0] = '\0';

	char * token = strtok(str, " ");
	while( token != NULL ) {
		
		if(strcmp(token, "_map_name_") == 0)
		{
			strcat(newStr, _map->name);
		}else if(strcmp(token, "_map_artist_") == 0)
		{
			strcat(newStr, _map->artist);
		}else if(strcmp(token, "_map_difficulty_") == 0)
		{
			sprintf(newStr, "%s%i", newStr, _map->difficulty);
		}else if(strcmp(token, "_map_time_") == 0)
		{
			sprintf(newStr, "%s%i:%i", newStr, _map->musicLength/60, _map->musicLength%60);
		}else if(strcmp(token, "_map_image_") == 0)
		{
			sprintf(newStr, "%s/%s", _map->folder, _map->imageFile);
		}else if(strcmp(token, "_map_highscore_") == 0)
		{
			sprintf(newStr, "%i", _map->highscore);
		}else if(strcmp(token, "_map_combo_") == 0)
		{
			sprintf(newStr, "%i", _map->combo);
		}else if(strcmp(token, "_map_miss_") == 0)
		{
			sprintf(newStr, "%i", _map->misses);
		}else if(strcmp(token, "_map_accuracy_") == 0)
		{
			sprintf(newStr, "%.2f", (1-_map->accuracy)*100);
		}else {
			strcat(newStr, token);
		}
		token = strtok(NULL, " ");
		
		if(token != 0)
			strcat(newStr, " ");
	}

	strcpy(str, newStr);
	free(newStr);
}

bool CSS_ObjectActive(CSS_Object* object)
{
	while(object)
	{
		if(!object->active)
			return false;
		object = &_pCSS->objects[object->parentObj];
	}
	return true;
}

CSS_Object * makeCSS_ObjectClone(CSS_Object object)
{
	_pCSS->objectCount++;
	_pCSS->objects = realloc(_pCSS->objects, sizeof(CSS_Object)*_pCSS->objectCount);
	
	CSS_Object * new = &_pCSS->objects[_pCSS->objectCount-1];

	*new = object;

	stringcopy(new->name, object.name);
	stringcopy(new->parent, object.parent);
	stringcopy(new->text, object.text);
	stringcopy(new->hintText, object.hintText);
	stringcopy(new->loadFile, object.loadFile);
	stringcopy(new->makeActive, object.makeActive);
	stringcopy(new->command, object.command);
	stringcopy(new->image.file, object.image.file);

	new->children = malloc(sizeof(int)*object.childrenCount);

	// printf("parent: %s\n", new->name);


	//variable replacement part

	if(new->text) //do more checks if maps points correctly
	{
		replaceTextVariables(new->text);
	}

	if(new->command)
	{
		replaceTextVariables(new->command);
	}

	if(new->image.file)
	{
		replaceTextVariables(new->image.file);
	}


	// printf("new name %s\n", new->name);
	if(!strcmp(new->name, "highscoreContainer"))
	{
		if(_map)
		{
			if(_map->highscore)
			{
				new->active = true;
			}
		}
	}

	if(!strcmp(new->text, "_map_rank_"))
	{
		if(_map)
		{
			if(_map->highscore)
			{
				new->image.state = CSSImage_rank;
				new->image.isCopy = true;
				new->value = _map->rank;
			}
		}
	}

	int index = new-_pCSS->objects;

	for(int i = 0; i < object.childrenCount; i++)
	{
		CSS_Object * child = makeCSS_ObjectClone(_pCSS->objects[object.children[i]]);
		child->parentObj = index;
		_pCSS->objects[index].children[i] = child-_pCSS->objects;

		if(_pCSS->objects[index].makeActive)
		{
			if(strcmp(_pCSS->objects[index].makeActive, child->name) == 0)
			{
				//copy id of child to parent makeActiveObj
				_pCSS->objects[index].makeActiveObj = _pCSS->objects[index].children[i];
			}
		}
	}

	return &_pCSS->objects[index];
}

CSS_Object * getCSS_ObjectPointer(char * name)
{
	if(!name || !_pCSS)
		return 0;
	
	int charBits = stringToBits(name);

	for(int i = 0; i < _pCSS->objectCount; i++)
	{
		if(_pCSS->objects[i].nameCharBits == charBits)
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
		strncpy(variable, object->text, 100);
	}else
	{
		strncpy(object->text, variable, 100);
	}
}

bool UISelected(char * name)
{
	if(!name)
		return false;
	
	CSS_Object *object = getCSS_ObjectPointer(name);

	if(!object)
		return false;
	
	return object->selected;
}

int UIValueInteractable(int variable, char * name)
{
	if(!name)
		return variable;
	
	CSS_Object *object = getCSS_ObjectPointer(name);

	if(!object)
		return variable;
	
	if(!object->selected)
	{
		object->value = variable;
		return variable;
	}

	return object->value;
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
	drawBackground();
	drawVignette();
	drawCSS(_pCSS->file);
	drawCursor();
}

void fMainMenu(bool reset)
{

	checkFileDropped();
	_musicLoops = true;
	ClearBackground(BLACK);
	drawBackground();

	drawVignette();

	if(UIBUttonPressed("settingsButton"))
	{
		// Switching to settings
		_pGameplayFunction = &fSettings;
		_pNextGameplayFunction = &fMainMenu;
		_transition = 0.1;
	}

	if(UIBUttonPressed("newmapButton"))
	{
		_pGameplayFunction = &fNewMap;
		_pNextGameplayFunction = &fMainMenu;
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
	if(reset)
	{
		return;
	}
	
	_musicPlaying = false;
	ClearBackground(BLACK);
	drawBackground();

	drawVignette();
	

	drawCSS("theme/settings.css");

	_settings.volumeGlobal = UIValueInteractable(_settings.volumeGlobal, "globalVolume");
	_settings.volumeMusic = UIValueInteractable(_settings.volumeMusic, "musicVolume");
	_settings.volumeSoundEffects = UIValueInteractable(_settings.volumeSoundEffects, "effectVolume");

	_settings.customNoteHeigth = UIValueInteractable(_settings.customNoteHeigth*100, "customNoteHeight")/100.0;
	_settings.noteOverlap = UIValueInteractable(_settings.noteOverlap*100, "noteOverlap")/100.0;
	_settings.zoom = UIValueInteractable(_settings.zoom, "zoomSlider");
	_settings.noteSize = UIValueInteractable(_settings.noteSize, "noteSizeSlider");
	_settings.offset = UIValueInteractable(_settings.offset, "offsetSlider");
	_settings.offset = UIValueInteractable(_settings.offset, "offsetBox");

	_settings.backgroundDarkness = UIValueInteractable(_settings.backgroundDarkness*255, "backgroundDarknessSlider") / 255.0;


	_settings.animations = UIValueInteractable(_settings.animations, "animationsToggle");
	_settings.customAssets = UIValueInteractable(_settings.customAssets, "customAssetsToggle");
	_settings.useMapZoom = UIValueInteractable(_settings.useMapZoom, "useMapZoomToggle");
	_settings.horizontal = UIValueInteractable(_settings.horizontal, "noteDirectionToggle");

	UITextBox(_playerName, "nameBox");

	if ( IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	{
		_pGameplayFunction = _pNextGameplayFunction;

		if(_pGameplayFunction == &fCountDown)
		{
			_pNextGameplayFunction = &fPlaying;
			fCountDown(true);
		}

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

	if (IsKeyPressed(KEY_ESCAPE))
	{
		if (_pNextGameplayFunction == &fPlaying)
		{
			_pGameplayFunction = &fCountDown;
			fCountDown(true);
			_transition = 0.1;
		}
		else
			_pGameplayFunction = _pNextGameplayFunction;
	}

	if(UIBUttonPressed("continueButton"))
	{
		if(_pNextGameplayFunction == &fEditor)
		{
			_fromEditor = true;
			for(_noteIndex = 0; _noteIndex < _amountNotes; _noteIndex++)
			{
				if(_papNotes[_noteIndex]->time > getMusicHead())
					break;
			}
		}
		_pNextGameplayFunction = &fPlaying;
		_pGameplayFunction = &fCountDown;
		fCountDown(true);
		_transition = 0.1;
	}

	UISetActive("retryButton", (_pNextGameplayFunction == &fPlaying || _pNextGameplayFunction == &fRecording));
	// UISetActive("editButton", (_pNextGameplayFunction == &fPlaying));
	UISetActive("saveButton", (_pNextGameplayFunction == &fEditor));

	if (UIBUttonPressed("saveButton"))
	{
		saveMap();
		_pGameplayFunction = _pNextGameplayFunction;
		// gotoMainMenu(false);
		_transition = 0.5;
	}

	if(UIBUttonPressed("editButton"))
	{
		_pGameplayFunction = &fEditor;
		if(_pNextGameplayFunction != &fEditor)
			fEditor(true);

		for(int i = 0; i < _amountNotes; i++)
			_papNotes[i]->hit = false;
	}

	if (_pNextGameplayFunction == &fPlaying && UIBUttonPressed("retryButton"))
	{
		_pGameplayFunction = fCountDown;
		_noteIndex = 0;
		_musicHead = 0;
		fCountDown(true);
		fPlaying(true);
		_transition = 0.1;
	}

	if (_pNextGameplayFunction == &fRecording && UIBUttonPressed("retryButton"))
	{
		_pGameplayFunction = _pNextGameplayFunction;
		_noteIndex = 0;
		_amountNotes = 0;
		_musicHead = 0;
		fRecording(true);
		_transition = 0.1;
	}

	if (UIBUttonPressed("exitButton"))
	{
		unloadMap();
		gotoMainMenu(false);
		_transition = 0.1;
	}
	drawCursor();
}


char **filesCaching = 0;

#ifdef __unix
void mapInfoLoading()
#else
// DWORD WINAPI *mapInfoLoading()
void mapInfoLoading()
#endif
{
	lockLoadingMutex();
	_loading++;
	unlockLoadingMutex();
	FilePathList files = LoadDirectoryFiles("maps/");
	static int oldAmount = 0;

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

	if(amount != oldAmount) //fixes crash when maps aren't the same
	{
		for(int i = 0; i < oldAmount; i++)
		{
			freeArray(filesCaching[i]);
		}

		oldAmount = 0;
	}
	
	char **newFiles = malloc(sizeof(char *) * amount);
	for(int i = 0, j = 0; i < files.count && j < amount; i++)
	{
		char str [100];
		snprintf(str, 100, "%s/map.data", files.paths[i]);
		if(FileExists(str))
		{
			newFiles[j] = malloc(100);
			strncpy(newFiles[j], files.paths[i], 100);
			j++;
		}
	}
	UnloadDirectoryFiles(files);

	if (_paMaps != 0)
	{	
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
				strncpy(filesCaching[mapIndex], filesCaching[j], 100);
				_paMaps[mapIndex] = _paMaps[j];
				_paMaps[j] = (Map){0};
				readScore(&_paMaps[mapIndex]);
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
			readScore(&_paMaps[mapIndex]);
		}

		// caching
		strncpy(filesCaching[mapIndex], newFiles[i], 100);

		mapIndex++;
	}
	lockLoadingMutex();
	_loading--;
	unlockLoadingMutex();
	_mapsCount = mapIndex;
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
	static int selectedMap = -1;
	static float selectMapTransition = 1;
	static bool selectingMods = false;
	_musicSpeed = 1;
	static char search[100];

	if (selectMapTransition < 1)
		selectMapTransition += GetFrameTime() * 10;
	if (selectMapTransition > 1)
		selectMapTransition = 1;
	checkFileDropped();
	// if (_mapRefresh)
	// {
	// 	createThread((void *(*)(void *))mapInfoLoading, 0);
	// 	_mapRefresh = false;
	// }
	ClearBackground(BLACK);
	drawBackground();

	if (selectingMods)
	{
		_playMenuMusic = true;
		_musicPlaying = false;
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

	_map = 0; //fixes stuff for load map
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
	// menuScroll = clamp(menuScroll, -.2 * floor(amount / 2), 0);

	if(UIBUttonPressed("modsButton"))
	// if (interactableButton("Mods", 0.03, getWidth() * 0.2, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		selectingMods = true;
	}

	if (IsKeyPressed(KEY_ESCAPE) || UIBUttonPressed("backButton"))
	//interactableButton("Back", 0.03, getWidth() * 0.05, getHeight() * 0.05, getWidth() * 0.1, getHeight() * 0.05))
	{
		gotoMainMenu(false);
		_pGameplayFunction = &fMainMenu;
		_transition = 0.1;
	}

	char oldSearch[100];
	strcpy(oldSearch, search);
	UITextBox(search, "searchBox");

	if(strcmp(oldSearch, search))
	{
		//search has updated, updating maps

		CSS_Object * mapArrayObject = getCSS_ObjectPointer("maps");

		for(int i = 0; i < mapArrayObject->arrayChildCount; i++)
		{
			// mapArrayObject->arrayChildren[i].key

			bool missingLetter = false;
			char str[100];
			// snprintf(str, 100, "%s - %s", _paMaps[i].name, _paMaps[i].artist);
			strcpy(str, mapArrayObject->arrayChildren[i].key);
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

			mapArrayObject->arrayChildren[i].enabled = !missingLetter;
		}

		updateArrayChildren(mapArrayObject);
	}

	CSS_Object * searchText = getCSS_ObjectPointer("searchText");
	CSS_Object * searchBox = getCSS_ObjectPointer("searchBox");

	if(searchText && searchBox)
		searchText->active = search[0] == '\0' && !searchBox->selected &&
			!mouseInRect((Rectangle){.x=searchBox->x*getWidth(), .y=searchBox->y*getHeight(), .width=searchBox->width*getWidth(), .height=searchBox->height*getHeight()});

	// if(selectedMap == -1)
	// {
	// 	_playMenuMusic = true;
	// 	_musicPlaying = false;
	// 	_musicFrameCount = 1;
	// }

	// draw map button
	Rectangle mapSelectRect = (Rectangle){.x = 0, .y = getHeight() * 0.13, .width = getWidth(), .height = getHeight()};
	startScissor(mapSelectRect.x, mapSelectRect.y, mapSelectRect.width, mapSelectRect.height);
	int mapCount = -1;
	// for (int i = 0; i < amount; i++)
	// {
	// 	// draw maps
	// 	if (search[0] != '\0')
	// 	{
	// 		bool missingLetter = false;
	// 		char str[100];
	// 		snprintf(str, 100, "%s - %s", _paMaps[i].name, _paMaps[i].artist);
	// 		int stringLength = strlen(str);
	// 		for (int j = 0; j < 100 && search[j] != '\0'; j++)
	// 		{
	// 			bool foundOne = false;
	// 			for (int k = 0; k < stringLength; k++)
	// 			{
	// 				if (tolower(search[j]) == tolower(str[k]))
	// 					foundOne = true;
	// 			}
	// 			if (!foundOne)
	// 			{
	// 				missingLetter = true;
	// 				break;
	// 			}
	// 		}
	// 	}

	// 	mapCount++;
	// 	int x = getWidth() * 0.02 + getWidth() * 0.32 * (mapCount % 3);
	// 	Rectangle mapButton = (Rectangle){.x = x, .y = menuScrollSmooth * getHeight() + getHeight() * ((floor(i / 3) > floor(selectedMap / 3) && selectedMap != -1 ? 0.3 : 0.225) + 0.3375 * floor(mapCount / 3)), .width = getWidth() * 0.3, .height = getHeight() * 0.3};
		
	// 	if(mapButton.y + mapButton.height < 0 || mapButton.y > getHeight())
	// 		continue;

	// 			setCSS_Variable("mapname", _paMaps[i].name);
	// 	setCSS_Variable("artist", _paMaps[i].artist);
	// 	setCSS_VariableInt("difficulty", _paMaps[i].difficulty);

	// 	CSS_Variable * nameAndArtistVar = getCSS_Variable("mapname_artist");
	// 	if(nameAndArtistVar)
	// 		snprintf(nameAndArtistVar->value, 100, "%s - %s", _paMaps[i].name, _paMaps[i].artist);


	// 	CSS_Variable * musicLength = getCSS_Variable("musicLength");
	// 	if(musicLength)
	// 		snprintf(musicLength->value, 100, "%i:%i", _paMaps[i].musicLength/60, _paMaps[i].musicLength%60);

		
	// 	//todo: lord forgive me for what i've done
	// 	CSS_Object * mapImageObject = getCSS_ObjectPointer("mapImage");
	// 	if(mapImageObject)
	// 	{
	// 		mapImageObject->image = _paMaps[i].image;
	// 	}

	// 	drawContainer("mapContainer", mapButton.x, mapButton.y);

	// 	if(highScores[i] != 0)
	// 	{
	// 		CSS_Object * highScoreObject = getCSS_ObjectPointer("highscoreContainer");

	// 		if(highScoreObject)
	// 		{
	// 			setCSS_VariableInt("highscore", highScores[i]);
	// 			setCSS_VariableInt("combo", combos[i]);
	// 			setCSS_VariableInt("accuracy", 100*(1-accuracy[i]));
	// 			setCSS_VariableInt("misses", misses[i]);

	// 			drawContainer("highscoreContainer", mapButton.x, mapButton.y);

	// 			CSS_Object * rankObj = getCSS_ObjectPointer("rank");
	// 			if(rankObj)
	// 				drawRank(rankObj->x*getWidth()+mapButton.x, rankObj->y*getHeight()+mapButton.y, rankObj->width*getWidth(), rankObj->height * getWidth(), ranks[i]);
	// 		}
	// 	}

	// 	if(_paMaps[i].cpuImage.width == 0)
	// 	{
	// 		_paMaps[i].cpuImage.width = -1;
	// 		createThread((void *(*)(void *))loadMapImage, &_paMaps[i]);
	// 		_paMaps[i].cpuImage.width = -1;
	// 	}

	// 	if (selectedMap == i)
	// 	{
	// 		if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
	// 			selectedMap = -1;
			
	// 		startScissor(mapButton.x, mapButton.y, mapButton.width, mapButton.height*0.8+mapButton.height*0.5*selectMapTransition);
	// 		drawContainer("mapButtonsContainer", mapButton.x, mapButton.y);
	// 		endScissor();

	// 		bool playButton = UIBUttonPressed("playButton");
	// 		bool editorButton = UIBUttonPressed("editorButton");
	// 		bool exportButton = UIBUttonPressed("exportButton");

	// 		if(!mouseInRect(mapSelectRect))
	// 		{
	// 			playButton = false;
	// 			editorButton = false;
	// 			exportButton = false;
	// 		}
			
	// 		if (playButton || editorButton || exportButton)
	// 		{
	// 			_map = &_paMaps[i];
	// 			setMusicStart();
	// 			_musicHead = 0;
	// 			printf("selected map!\n");
	// 			_transition = 0.1;
	// 			_disableLoadingScreen = false;
	// 			_musicPlaying = false;

	// 			CSS_Object * mapImageObject = getCSS_ObjectPointer("mapImage");
	// 			if(mapImageObject)
	// 			{
	// 				mapImageObject->image = (Texture2D){0};
	// 			}

	// 			loadMap();
	// 		}
			
	// 		if (playButton)
	// 		{
	// 			_pNextGameplayFunction = &fPlaying;
	// 			_pGameplayFunction = &fCountDown;
	// 			fCountDown(true);
	// 			fPlaying(true);
	// 		}
	// 		if (editorButton)
	// 		{
	// 			_pNextGameplayFunction = &fEditor;
	// 			_pGameplayFunction = &fEditor;
	// 			fEditor(true);
	// 		}
	// 		if (exportButton)
	// 		{
	// 			_pGameplayFunction = &fExport;
	// 		}
	// 		DrawRectangleGradientV(mapButton.x, mapButton.y + mapButton.height, mapButton.width, mapButton.height * 0.05 * selectMapTransition, ColorAlpha(BLACK, 0.3), ColorAlpha(BLACK, 0));
	// 	}
	// 	else
	// 	{
	// 		// drawMapThumbnail(mapButton, &_paMaps[i], (highScores)[i], (combos)[i], (accuracy)[i], false);

	// 		if (IsMouseButtonReleased(0) && mouseInRect(mapButton) && mouseInRect(mapSelectRect))
	// 		{
	// 			playAudioEffect(_buttonSe);
	// 			selectedMap = i;
	// 			selectMapTransition = 0;
	// 			_musicFrameCount = 1;

	// 			// play music
	// 			char str[100];
	// 			snprintf(str, 100, "%s/%s", _paMaps[i].folder, _paMaps[i].musicFile);
	// 			loadMusic(&_paMaps[i].music, str, _paMaps[i].musicPreviewOffset);
	// 			_playMenuMusic = false;
	// 			_musicFrameCount = _paMaps[i].musicPreviewOffset * 48000 * 2;
	// 			_musicPlaying = true;
	// 			_disableLoadingScreen = true;
	// 		}

	// 		if(_paMaps[i].image.id == 0)
	// 		{
	// 			//load map image onto gpu(cant be loaded in sperate thread because opengl >:( )
	// 			if(_paMaps[i].cpuImage.width > 0)
	// 			{
	// 				_paMaps[i].image = LoadTextureFromImage(_paMaps[i].cpuImage);
	// 				UnloadImage(_paMaps[i].cpuImage);
	// 				_paMaps[i].cpuImage.width = -1;
	// 				if(_paMaps[i].image.id == 0)
	// 					_paMaps[i].image.id = -1; 
	// 				else
	// 					SetTextureFilter(_paMaps[i].image, TEXTURE_FILTER_BILINEAR);
	// 			}
	// 		}
	// 	}
	// }

	// CSS_Object * mapImageObject = getCSS_ObjectPointer("mapImage");
	// if(mapImageObject)
	// {
	// 	mapImageObject->image = (Texture2D){0};
	// }
	
	drawVignette();

	endScissor();

	DrawRectangleGradientV(0, getHeight()*0.8, getWidth(), getHeight()*0.21, ColorAlpha(BLACK, 0), ColorAlpha(BLACK, 0.8));


	if (selectedMap != -1 && _paMaps[selectedMap].name != 0)
	{
		char str[100];
		snprintf(str, 100, "%s - %s", _paMaps[selectedMap].name, _paMaps[selectedMap].artist);
		int textSize = measureText(str, getWidth() * 0.05);
		drawText(str, getWidth() * 0.9 - textSize, getHeight() * 0.92, getWidth() * 0.05, WHITE);
	}

	drawCursor();
}



void fExport(bool reset)
{
	_pGameplayFunction = &fMapSelect;
	makeMapZip(_map);
	char str[300];
	snprintf(str, 300, "%s/%s.zip", GetWorkingDirectory(), _map->name);
	SetClipboardText(str);
	resetBackGround();
	strncpy(_notfication, "exported map", 100);
	_transition = 0.5;
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
		strncpy(newMap.name, "name", 100);
		
		if(!newMap.artist)
			newMap.artist = malloc(100);
		strncpy(newMap.artist, "Artist", 100);

		if(!newMap.mapCreator)
			newMap.mapCreator = malloc(100);
		strncpy(newMap.mapCreator, _playerName, 100);

		if(!newMap.folder)
			newMap.folder = malloc(100);
		newMap.folder[0] = '\0';
		newMap.bpm = 100;
		newMap.beats = 4;
		newMap.zoom = 7;
		newMap.id = rand();
		newMap.difficulty = 5;
	}
	
	ClearBackground(BLACK);
	drawBackground();

	drawVignette();

	drawCSS("theme/newMap.css");


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
		snprintf(str, 100, "maps/%s/song%s", newMap.name, pMusicExt);
		FILE *file = fopen(str, "wb");
		fwrite(pMusic, pMusicSize, 1, file);
		fclose(file);

		newMap.musicFile = malloc(100);
		snprintf(newMap.musicFile, 100, "/song%s", pMusicExt);

		snprintf(str, 100, "maps/%s/image.png", newMap.name);
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
		_playMenuMusic = false;
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
			files = malloc(100);
			char * part = calloc( 100, sizeof(char));
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
						strncpy(files[file], part, 100);
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
				strncpy(pMusicExt, ext, 50);
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
				strncpy(pMusicExt, ext, 50);
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

	static int selectionEnd = -1;

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

	if(mouseInRect(rect) && IsMouseButtonPressed(0))
	{
		//calculate selectionEnd pos
		int fullLength = measureText(str, fontSize*screenSize);
		int x = rect.x + rect.width / 2 - fullLength / 2;
		float textPos = ((GetMouseX()-x) / (float)fullLength);
		selectionEnd = textPos * strlen(str) + 1;

		if(selectionEnd < 0)
			selectionEnd = 0;

		if(selectionEnd > strlen(str))
			selectionEnd = strlen(str);
	}

	if (*selected && selectionEnd != -1)
	{
		//draw selection
		char tmpStr[100];
		strncpy(tmpStr, str, 100);
		tmpStr[cursor] = '\0';
		int cursorLength = measureText(tmpStr, fontSize*screenSize);

		strncpy(tmpStr, str, 100);
		tmpStr[selectionEnd] = '\0';
		int selectionLength = measureText(tmpStr, fontSize*screenSize);

		int lowest = cursorLength > selectionLength ? selectionLength : cursorLength;
		int highest = cursorLength < selectionLength ? selectionLength : cursorLength;

		int fullLength = measureText(str, fontSize*screenSize);
		int x = rect.x + rect.width / 2 - fullLength / 2;
		DrawRectangle(x+lowest, rect.y, highest-lowest, rect.height, ColorAlpha(BLACK, 0.3));
	}

	if (*selected)
	{
		*selected = true;
		char c = GetCharPressed();
		while (c != 0)
		{
			if(strlen(str) > 99)
			{
				break;
			}

			if(selectionEnd != -1)
			{
				int lowest = cursor > selectionEnd ? selectionEnd : cursor;
				int highest = cursor < selectionEnd ? selectionEnd : cursor;

				char strCopy[100];
				strncpy(strCopy, str, 100);
				strCopy[lowest] = '\0';
				strCopy[highest-1] = '\0';

				snprintf(str, 100, "%s%s", strCopy, strCopy+highest);
				selectionEnd = -1;
			}

			char strPart1[100];
			strncpy(strPart1, str, 100);
			strPart1[cursor] = '\0';

			char strPart2[100];
			strncpy(strPart2, str+cursor, 100);

			snprintf(str, 100, "%s%c%s",strPart1, c, strPart2);
			str[strlen(str)+2] = '\0';

			cursor++;
			c = GetCharPressed();
			blinkingTimeout = GetTime();
		}

		if(IsKeyPressed(KEY_LEFT))
		{
			if(IsKeyDown(KEY_LEFT_SHIFT))
			{
				if(selectionEnd == -1)
					selectionEnd = cursor;
			}else
				selectionEnd = -1;
			
			cursor--;
			if(cursor < 0)
				cursor = 0;
			
			blinkingTimeout = GetTime();
		}

		if(IsKeyPressed(KEY_RIGHT))
		{
			if(IsKeyDown(KEY_LEFT_SHIFT))
			{	
				if(selectionEnd == -1)
					selectionEnd = cursor;
			}else
				selectionEnd = -1;
			
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

		if (IsKeyPressed(KEY_BACKSPACE) && cursor != 0 && selectionEnd == -1)
		{
			int prev = cursor;
			cursor--;
			if(cursor < 0)
				cursor = 0;
			
			char strCopy[100];
			strncpy(strCopy, str, 100);
			strCopy[cursor] = '\0';

			snprintf(str, 100, "%s%s",strCopy, strCopy+prev);
			blinkingTimeout = GetTime();
		}

		if (IsKeyPressed(KEY_DELETE) && cursor != strlen(str) && selectionEnd == -1)
		{
			char strCopy[100];
			strncpy(strCopy, str, 100);
			strCopy[cursor] = '\0';

			char strCopy2[100];
			strncpy(strCopy2, str+cursor+1, 100);

			snprintf(str, 100, "%s%s",strCopy, strCopy2);
			blinkingTimeout = GetTime();
		}

		if (selectionEnd != -1 && IsKeyDown(KEY_LEFT_CONTROL) && (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_X)))
		{
			char strCopy[100];
			int lowest = cursor > selectionEnd ? selectionEnd : cursor;
			int highest = cursor < selectionEnd ? selectionEnd : cursor;

			strncpy(strCopy, str, 100);
			strCopy[highest] = '\0';

			SetClipboardText(strCopy+lowest);
		}

		if((IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_DELETE) || (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_X)))
			&& selectionEnd != -1)
		{
			int lowest = cursor > selectionEnd ? selectionEnd : cursor;
			int highest = cursor < selectionEnd ? selectionEnd : cursor;

			char strCopy[100];
			strncpy(strCopy, str, 100);
			strCopy[lowest] = '\0';
			strCopy[highest-1] = '\0';

			snprintf(str, 100, "%s%s", strCopy, strCopy+highest);
			selectionEnd = -1;
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_BACKSPACE))
		{
			char strCopy[100];
			strncpy(strCopy, str+cursor, 100);
			strncpy(str, strCopy, 100);
			cursor = 0;
			blinkingTimeout = GetTime();
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_DELETE))
		{
			str[cursor] = '\0';
			blinkingTimeout = GetTime();
		}

		if (IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_V))
		{
			char * clipboard = (char*) GetClipboardText();

			if(!clipboard)
				return;
			
			char tmpStr[100];
			strncpy(tmpStr, str, 100);
			tmpStr[cursor] = '\0';

			char ogString[100];
			strncpy(ogString, str+cursor, 100);

			snprintf(str, 100, "%s%s%s", tmpStr, clipboard, ogString);

			// free(clipboard);
		}
	}

	if (*selected)
	{
		//draw cursor
		char tmpStr[100];
		strncpy(tmpStr, str, 100);
		tmpStr[cursor] = '\0';
		int cursorLength = measureText(tmpStr, fontSize*screenSize);

		int fullLength = measureText(str, fontSize*screenSize);
		int x = rect.x + rect.width / 2 - fullLength / 2;
		if((int)(GetTime()*2) % 2 == 0 || GetTime() - blinkingTimeout < 0.5)
			DrawRectangle(x+cursorLength, rect.y, getWidth()*0.003, rect.height, DARKGRAY);
	}

	if (*selected && ((!mouseInRect(rect) && IsMouseButtonReleased(0)) || IsKeyPressed(KEY_ENTER)))
	{
		*selected = false;
		cursor = 0;
		selectionEnd = -1;
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