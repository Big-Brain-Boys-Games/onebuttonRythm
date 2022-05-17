#include <stdio.h>
#include <stdlib.h>

#ifdef __unix
#include <pthread.h>
pthread_mutex_t _loadingMutex;
#else
#include "windowsDefs.h"
#include <windows.h>
HANDLE _loadingMutex;
#endif

#ifdef __unix
void createThread(void *(*func)(void *), void *args)
#else
void createThread(DWORD WINAPI *(*func)(void *), void *args)
#endif
{
#ifdef __unix
    pthread_t id = 0;
    int tmp = pthread_create(&id, NULL, func, args);
#else
    CreateThread(NULL, 0, func, args, 0, NULL);
#endif

    return;
}

void LoadingMutexInit()
{
    #ifdef __unix
        pthread_mutex_init(&_loadingMutex, NULL);
    #else
        _loadingMutex = CreateMutex( NULL, FALSE, NULL);
    #endif
}

void lockLoadingMutex()
{
    #ifdef __unix
        pthread_mutex_lock(&_loadingMutex);
    #else
        WaitForSingleObject(_loadingMutex, INFINITE);
    #endif
}

void unlockLoadingMutex()
{
    #ifdef __unix
        pthread_mutex_unlock(&_loadingMutex);
    #else
        ReleaseMutex(_loadingMutex);
    #endif
}