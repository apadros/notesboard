#ifndef TEXT_H
#define TEXT_H

#include "apad_base_types.h"
#include "apad_intrinsics.h."
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_time.h"

// ******************** Notes ******************** //

const ui16 NoteMinWidth = 200;
const ui16 NoteTextHeight = 15;
const ui16 NoteTextBorderOffset = NoteTextHeight;
const ui16 NoteMinHeight = NoteTextHeight + NoteTextBorder * 2;

const ui16 NoteTitleTextHeight = NoteTextHeight * 1.5f;

struct note {
	f32       width;
	text_body title; // Edges in canvas space
	text_body text;  // Edges in canvas space
};
// @TODO - Need a function for overall overlap checks
// @TODO - Need a function to get a text_body's edges / rectangle based on render dimensions

#define BeginNotesMemoryLoop(_varID) { ForAll(state.notes.memory.size / sizeof(note)) { \
																		     note* _varID = (note*)state.notes.memory.memory + it;
#define EndNotesMemoryLoop() 				 } }

#define BeginNotesLoop(_varID) BeginNotesMemoryLoop(_varID)
#define EndNotesLoop() 				 EndNotesMemoryLoop()

note*   CreateNote(vector pos, const char* title, const char* text);
note*   GetCurrentNote();

struct note_text_render_data {
	rectangle title;
	rectangle titleEdges;
	rectangle text;
	rectangle textEdges;
}    GetNoteTextRenderData(note* n); // Width == 0 if no text present for both text and title
bool NoteIsBeingUpdated();
bool NoteHasTitle(note* n);
bool NoteMemoryIsInUse(note* n);

// ******************** Misc ******************** //

const ui8  TitleBarHeight = 100;
const ui8  TitleBarTextHeight = TitleBarHeight / 3;

const ui8  ToolbarWidth = 150;
const ui8  ToobalIconWidth = ToolbarWidth * 0.5f;
const ui8  ToolbarTextHeight = 10;
const ui8  ToolVerticalSpaceBetweenIcons = ToolbarTextHeight * 2;

const ui8  TopMenuHeight = 50;
const ui16 TopMenuButtonWidth = 200; // In viewport space
const ui8  TopMenuTextHeight = (f32)TopMenuHeight / 2;

const ui16 ColourPanelWidth = 400;  // In viewport space
const ui16 ColourPanelHeight = 400; // In viewport space
const f32  ColourWheelSizeMult = 0.5f;
const f32  ColourWheelVerticalCenterMult = 0.7f;
const ui8  ColourWheelVertices = 36;

program_unique struct {
	struct {
		vector translation; // In viewport space, applied post scaling, therefore must be scaled
		f32    scale = 1.0f;
	} canvas; // Treated as the GL projeciton matrix, initially takes up entire viewport, including title and tool bars
	
	struct {
		rectangle background; // In viewport space
		text_body text;
	} 					titleBar;
	
	struct {
		rectangle 		background;
		struct {
			f32         left;
			const char* text;
		} 						buttons[3];
	} 							topMenu;
	
	struct {
		rectangle background; // In viewport space

		struct {
			rectangle 	background;
			const char* text;
			ui16        textBottom;
		} 						buttons[4];
		ui16          textHeight;
	} 							toolBar;
	
	struct {
		bool 			display;
		rectangle frame;
		vector    selection;
		bool      updatingSelection;
		f32       sliderCenterY;
		bool      updatingSlider;
		text_body red;
		text_body green;
		text_body blue;
	} 					colourPanel;

	struct {
		memory_block memory;
		note* 			 selected;
		bool         moving;
		bool 				 justCreated;
	} 						 notes;

	struct { // All vectors in viewport space
		vector pos;
		vector translation;
		bool   leftDown;
		bool   lastLeftDown;
	  bool   rightDown;
	} 			 mouse;
	
} state;

// Misc
#define GetTitleBar()    (&state.titleBar)
#define GetToolBar() 		 (&state.toolBar)
#define GetTopMenu() 		 (&state.topMenu)
bool    MouseIsWithinToolbar();
bool    MouseLeftDownThisFrame();
bool    MouseOverlapsCanvas(rectangle& r);
bool    MouseOverlapsGUI(rectangle& r);
bool    TitleIsBeingUpdated();

// Colour panel
#define 	GetColourPanel() \
						(&state.colourPanel)
rectangle GetColourPanelSliderRectangle();
rectangle GetColourPanelWheelRectangle();

// Space and projection matrix stuff
vector  ConvertToCanvasSpace(f32 x, f32 y);
vector  ConvertToCanvasSpace(vector pos);
vector  ConvertToViewportSpace(vector pos);
void 		SetGUIProjectionMatrix();
void 		SetCanvasProjetionMatrix();

#endif