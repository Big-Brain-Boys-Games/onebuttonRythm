
 #ifdef __cplusplus
 #define EXTERNC extern "C"
 #else
 #define EXTERNC
 #endif
EXTERNC void apiInit();
EXTERNC void apiUpdate();
EXTERNC void submitScore(int mapID, int points, int * rank);
#undef EXTERNC