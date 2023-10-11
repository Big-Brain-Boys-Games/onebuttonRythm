#include "commands.h"
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define EXTERN_FILES
#define EXTERN_GAMEPLAY
#define EXTERN_MAIN

#include "gameplay/gameplay.h"
#include "gameplay/playing.h"
#include "gameplay/editor.h"
#include "gameplay/menus.h"
#include "main.h"
#include "files.h"

typedef struct Command{
    void (*function)(char*);
    char * name;
} Command;

void commandPrint(char * cmd)
{
    printf("Print: %s\n", cmd);
}

void commandError(char * cmd)
{
    printf("Error: %s\n", cmd);
}

Map * getMap(char * name)
{
    if(name == 0)
        return 0;

    for(int i = 0; i < _mapsCount; i++)
    {
        if(strcmp(_paMaps[i].name, name) == 0)
        {
            return &_paMaps[i];
        }
    }
    
    return 0;
}

void commandPlay(char * cmd)
{
    bool countDown = true;
    bool reset = true;

    char * argument = strtok(cmd, " ");

    char * map = malloc(100);
    map[0] = '\0';
    bool NoMap = true;

    while( argument != NULL)
    {
        if(argument[0] == "-")
        {
            //is flag, check for all flags
            if(strcmp(argument, "-noCountDown") == 0)
            {
                countDown = false;
            }
            if(strcmp(argument, "-noReset") == 0)
            {
                reset = false;
            }
        }else
        {
            if(map[0] != '\0')
                strcat(map, " ");
            strcat(map, argument);
            NoMap = false;
        }

        argument = strtok(NULL, " ");
    }

    if(NoMap)
    {
        commandParser("error no map provided");
        return;
    }

    Map * pMap = getMap(map);

    if(pMap == 0)
    {
        char errormsg[100];
        strcpy(errormsg, "error Map doesn't exist");
        commandParser(errormsg);
        return;
    }

    _map = pMap;

    _pNextGameplayFunction = &fPlaying;
    _pGameplayFunction = &fPlaying;
    
    if(countDown)
    {
        _pGameplayFunction = &fCountDown;

        if(reset)
            fCountDown(true);
    }

    if(reset)
        fPlaying(true);
    _transition = 0.1;
}

void commandEdit(char * cmd)
{
    bool reset = true;

    char * argument = strtok(cmd, " ");

    char * map = malloc(100);
    map[0] = '\0';
    bool NoMap = true;

    while( argument != NULL)
    {
        if(argument[0] == "-")
        {
            //is flag, check for all flags
            if(strcmp(argument, "-noReset") == 0)
            {
                reset = false;
            }
        }else
        {
            if(map[0] != '\0')
                strcat(map, " ");
            strcat(map, argument);
            NoMap = false;
        }

        argument = strtok(NULL, " ");
    }

    if(NoMap)
    {
        commandParser("error no map provided");
        return;
    }

    Map * pMap = getMap(map);

    if(pMap == 0)
    {
        char errormsg[100];
        strcpy(errormsg, "error Map doesn't exit");
        commandParser(errormsg);
        return;
    }

    _map = pMap;

    _pNextGameplayFunction = &fPlaying;
    _pGameplayFunction = &fEditor;
    fEditor(true);
    _transition = 0.1;
}

void commandExport(char * cmd)
{

    char * argument = strtok(cmd, " ");

    char * map = malloc(100);
    map[0] = '\0';
    bool NoMap = true;

    while( argument != NULL)
    {
        if(argument[0] == "-")
        {
            //is flag, check for all flags

            //no arguments :P
        }else
        {
            if(map[0] != '\0')
                strcat(map, " ");
            strcat(map, argument);
            NoMap = false;
        }

        argument = strtok(NULL, " ");
    }

    if(NoMap)
    {
        commandParser("error no map provided");
        return;
    }

    Map * pMap = getMap(map);

    if(pMap == 0)
    {
        char errormsg[100];
        strcpy(errormsg, "error Map doesn't exist");
        commandParser(errormsg);
        return;
    }

    _map = pMap;

    _pNextGameplayFunction = &fMainMenu;
    _pGameplayFunction = &fExport;
    _transition = 0.1;
}

Command _commands[] = {
    {commandPrint, "print"},
    {commandError, "error"},
    {commandPlay, "play"},
    {commandEdit, "edit"},
    {commandExport, "export"},
    {commandPlay, "play"},
};

void commandParser(char * line)
{
    if(!line)
        return;

    int length = strlen(line);

    if(length < 1)
        return;

    if(length >= 99)
        return; 

    char str[100];
    strncpy(str, line, 100);

    for(int i = 0; i < length; i++)
        if(str[i] == ' ')
        {
            str[i] = '\0';
            break;
        }
    
    char * command = str;

    printf("command: %s\n", line);

    int commandsCount = sizeof(_commands) / sizeof(_commands[0]);

    for(int i = 0; i < commandsCount; i++)
    {
        if(_commands[i].name[0] != command[0])
            continue;
        
        if(strcmp(_commands[i].name, command))
            continue;

        _commands[i].function(command+strlen(str)+1);

        return;
    }

    printf("command not found: %s\nFull Line: %s\n", command, line);
}