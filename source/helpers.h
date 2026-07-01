#ifndef TEXT_H
#define TEXT_H

#include "apad_base_types.h"
#include "apad_intrinsics.h."
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_time.h"

// ******************** Text ******************** //

// @TODO - Export to APAD API?
struct text_body {
	memory_stack memory;
	bool         specialCharsAllowed; // Bullet points and new lines
	rectangle    background;
};

// @TODO - Export to APAD API?
text_body AllocateTextBody(bool allowSpecialChars, f32 left, f32 bottom, f32 width, f32 height);
void 			ClearTextBody(text_body& tb);
void      FreeTextBody(text_body& tb);
ui32 			GetTextLength(text_body& tb);
char*     GetTextStart(text_body& tb);
void 			InsertText(char* string, ui32 length, text_body& tb, ui32 pos);
bool      TextBodyIsValid(text_body& tb);

void 	BeginWriting(text_body& text, rectangle& containerBackground, f32 textHeight, bool leftAligned /* If false assumed to be centered */); // By default will place cursor offset at the end of the text body
void 	EndWriting();

void 	 AddText(char c); // Will add to current cursor position
char*  FindChar(char c, ui16 pos, bool scanForward); // Will return Null if not found
ui16   GetCharOffset(char* c);
vector GetTextRenderDimensions(char* text, ui32 length, f32 height); // Will return a minimum y of height even if no text present, by x will equal 0
void 	 MoveCursor(si8 offset);
void   RemoveChar(text_body& tb, ui32 pos); // Will remove a single char after pos
bool 	 TextIsBeingWritten();

// ******************** Notes ******************** //

const ui16 NoteMinWidth = 200;
const ui16 NoteTextHeight = 15;
const ui16 NoteTextBorder = NoteTextHeight;
const ui16 NoteMinHeight = NoteTextHeight + NoteTextBorder * 2;

const ui16 NoteTitleTextHeight = NoteTextHeight * 1.5f;

struct note {
	rectangle background; // In canvas space
	text_body title;
	text_body text;
};

#define BeginNotesMemoryLoop(_varID) { ForAll(state.notes.memory.size / sizeof(note)) { \
																		     note* _varID = (note*)state.notes.memory.memory + it;
#define EndNotesMemoryLoop() 				 } }

#define BeginNotesLoop(_varID) BeginNotesMemoryLoop(_varID)
#define EndNotesLoop() 				 EndNotesMemoryLoop()

note*   CreateNote(vector pos, const char* title, const char* text);
note*   GetCurrentNote();

struct note_text_render_data {
	rectangle title;
	rectangle titleContainer;
	rectangle text;
	rectangle textContainer;
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

const f32  CursorBlinkFullLength = 1.5f;

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
		text_body* textBody;
		rectangle* containerBackground;
		vector     cursorPos;
		ui16       cursorOffset; // 0-based
		f32        textHeight;
		b8         leftAligned;
		f32        cursorBlinkTime;
		f32        cursorAlpha;
	} 					 textUpdate;
	
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
void 		SetCursorPos(f32 x, f32 y);
bool    TitleIsBeingUpdated();
f32 		UI8ColourToF32(ui8 u);

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


// Rendering
void 			DrawRectangleBorder(f32 left, f32 bottom, f32 width, f32 height, ui8 r, ui8 g, ui8 b);
void 			DrawCircleBorder(f32 centerX, f32 centerY, f32 radius, ui8 lineWidth, ui8 r, ui8 g, ui8 b);
void 			DrawRectangle(f32 left, f32 bottom, f32 width, f32 height, ui8 r, ui8 g, ui8 b);
rectangle RenderText(char* text, ui32 length, f32 x, f32 y, f32 height, bool center);

#endif