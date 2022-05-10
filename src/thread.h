struct thread;
typedef struct thread thread;

#ifdef __unix
void createThread(void *(*func)(void*), void * args);
#else
#include "windowsDefs.h"
#include <windows.h>
void createThread(DWORD WINAPI*(*func)(void*), void * args);
#endif