#ifndef GAME_EDITOR
#define GAME_EDITOR

#include "../files.h"

#ifdef EXTERN_EDITOR

    extern bool _showSettings;
    extern bool _showNoteSettings;
    extern bool _showAnimation;
    extern bool _showTimingSettings;

    extern TimingSegment * _paTimingSegment;
    extern int _amountTimingSegments;

    extern Note **_selectedNotes;
    extern int _amountSelectedNotes;
    extern int _barMeasureCount;

#endif


TimingSegment getTimingSignature(float time);
TimingSegment * addTimingSignature(float time, int bpm);
void removeTimingSignature(float time);

void fEditorSongSettings(bool reset);
void fEditorTimingSettings (bool reset);
void fEditorNoteSettings(bool reset);
void fEditorAnimation (bool reset);
void fEditor(bool reset);
void removeNote(int index);
int newNote(float time);
void removeSelectedNote(int index);
void addSelectNote(int note);

#endif