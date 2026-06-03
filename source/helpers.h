#ifndef TEXT_H
#define TEXT_H

#include "apad_base_types.h"
#include "apad_intrinsics.h."
#include "apad_maths.h"
#include "apad_time.h"

const ui16 NoteMinWidth = 150;
const ui16 NoteTextHeight = 15;
const ui16 NoteTextBorder = NoteTextHeight;
const f32  QuickClickTime = 0.2; // Seconds

struct note {
	rectangle background;
	char*     title;
	char*     text;
};

program_unique struct {
	// Temporary write box
	struct {
		ui16         left;
		ui16         bottom;
		memory_stack memory;
	} writeBox;

	struct {
		rectangle background;

		struct {
			rectangle 	background;
			const char* text;
			ui16        textBottom;
		} 						buttons[3];
		ui16          textHeight;
	} toolbar;

	struct {
		memory_stack memory;
		note* 			 selectedByMouse;
		bool         moved; // To check whether to allow text writing
		memory_stack textMemory; // To push text being written onto. Once done, allocated onto specific note.
		note*        beingWritten;
	} notes;

	struct {
		ui16 				x;
		ui16 				y;
		ui16 				lastX;
		ui16 				lastY;
		bool 				leftDown;
		time_marker leftDownTime;
		bool        leftQuickClick;
	} mouse;
} state;

struct text_box {
	rectangle edges;
	ui16      cursorLeft;
};
text_box WriteText(const char* string, ui16 x, ui16 y, ui8 height);

// Temp text writing
void  AddTempWriting(char c);
void  BeginTempWriting(ui16 left, ui16 bottom);
char* EndTempWriting();
bool  TempTextIsBeingWritten();
bool  TempTextHasBeenWritten();

// Notes
#define BeginNotesLoop(_varID) { \
					ForAll(state.notes.memory.size / sizeof(note)) { \
						auto* _varID = (note*)state.notes.memory.memory + it;
#define EndNotesLoop() } }
point   GetNoteTextStart(note* n);
void 		EndNoteWriting();

// Misc
void 		DrawRectangle(ui16 left, ui16 bottom, ui16 width, ui16 height, ui8 r, ui8 g, ui8 b);
f32 		UI8ColourToF32(ui8 u);
#define UnpackDimensions(_struct) (_struct).left, (_struct).bottom, (_struct).width, (_struct).height

#endif