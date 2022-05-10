#include <stdio.h>
#include <stdlib.h>

#ifdef __unix
    #include <pthread.h>
#else
    #include "windowsDefs.h"
    #include <windows.h>
#endif


#ifdef __unix
    void createThread(void *(*func)(void*), void * args)
#else
    void createThread(DWORD WINAPI *(*func)(void*), void * args)
#endif
{
    #ifdef __unix
        int id = 0;
        pthread_create(&id, NULL, func, NULL);
    #else
        CreateThread(NULL, 0, func, args, 0, NULL);
    #endif

    return;
}