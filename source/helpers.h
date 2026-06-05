#ifndef TEXT_H
#define TEXT_H

#include "apad_base_types.h"
#include "apad_intrinsics.h."
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_time.h"

const ui8  TitleBarHeight = 100;
const ui8  TitleBarTextHeight = TitleBarHeight / 3;

const ui8  ToolbarWidth = 150;
const ui8  ToobalIconWidth = ToolbarWidth * 0.5f;
const ui8  ToolbarTextHeight = 10;
const ui8  ToolVerticalSpaceBetweenIcons = ToolbarTextHeight * 2;

const ui16 NoteMinWidth = 150;
const ui16 NoteTextHeight = 15;
const ui16 NoteTextBorder = NoteTextHeight;
const ui16 NoteMinHeight = NoteTextHeight + NoteTextBorder * 2;
const f32  QuickClickTime = 0.2; // Seconds

struct note {
	rectangle 	 background;
	char*     	 title;
	memory_stack textMemory;
};

program_unique struct {
	struct {
		rectangle 	 background;
		memory_stack textMemory;
	} titleBar;
	
	struct {
		memory_stack* textMemory;
		ui16 					cursorX;
		ui16 					cursorY;
		ui16 					cursorHeight;
	} 							textUpdate;
	
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
void 		 AddText(char c);
void 		 BeginWriting(memory_stack* textMemory, ui16 cursorHeight);
bool 		 TextIsBeingWritten();
void 		 EndWriting();
text_box WriteText(const char* string, ui16 x, ui16 y, ui8 height, bool center);

// Notes
#define BeginNotesLoop(_varID) { \
					ForAll(state.notes.memory.size / sizeof(note)) { \
						auto* _varID = (note*)state.notes.memory.memory + it;
#define EndNotesLoop() } }

// Misc
void 		DrawRectangle(ui16 left, ui16 bottom, ui16 width, ui16 height, ui8 r, ui8 g, ui8 b);
void 		SetCursorPos(ui16 x, ui16 y);
f32 		UI8ColourToF32(ui8 u);
#define UnpackDimensions(_struct) (_struct).left, (_struct).bottom, (_struct).width, (_struct).height

#endif