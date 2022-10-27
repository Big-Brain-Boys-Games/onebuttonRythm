#include "files.h"

TimingSegment getTimingSignature(float time);
TimingSegment * addTimingSignature(float time, int bpm);
void removeTimingSignature(float time);
void fEditor(bool reset);
void removeNote(int index);
int newNote(float time);
void removeSelectedNote(int index);
void addSelectNote(int note);