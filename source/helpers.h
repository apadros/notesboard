#ifndef TEXT_H
#define TEXT_H

#include "apad_base_types.h"
#include "apad_gui.h"
#include "apad_intrinsics.h."
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_time.h"
#include "apad_win32_gui.h"

// ******************** Notes ******************** //

const ui16 NoteMinWidth = 200;
const ui16 NoteTextHeight = 15;
const ui16 NoteTextBorder = NoteTextHeight;
const ui16 NoteMinHeight = NoteTextHeight + NoteTextBorder * 2;

const ui16 NoteTitleTextHeight = NoteTextHeight * 1.5f;

struct note {
	text_body title; // Edges in canvas space
	text_body text;  // Edges in canvas space
};

#define BeginNotesMemoryLoop(_varID) { ForAll(state.notes.memory.size / sizeof(note)) { \
																		     note* _varID = (note*)state.notes.memory.memory + it;
#define EndNotesMemoryLoop() 				 } }

#define BeginNotesLoop(_varID) BeginNotesMemoryLoop(_varID)
#define EndNotesLoop() 				 EndNotesMemoryLoop()

note*   CreateNote(vector pos, const char* title, const char* text);
note*   GetCurrentNote();
note* 	SetCurrentNote(note* n); // Set n to Null to deselect current note

rectangle GetNoteOverallRectangle(note* n);
bool 			NoteIsBeingUpdated();
bool 		  NoteTextIsBeingUpdated();
bool 			NoteTitleIsBeingUpdated();
bool 		  NoteHasTitle(note* n);
bool 		  NoteMemoryIsInUse(note* n);
void 		  UpdateNoteContainers(note* n); // Call after any updates to either text_body

// ******************** Misc ******************** //

const ui8  TitleBarHeight = 100;
const ui8  TitleBarTextHeight = TitleBarHeight / 3;

const ui8  ToolbarWidth = 150;
const ui8  ToobalIconWidth = ToolbarWidth * 0.5f;
const ui8  ToolbarTextHeight = 10;
const ui8  ToolVerticalSpaceBetweenIcons = ToolbarTextHeight * 2;

const ui8  TopMenuHeight = 50;
const ui16 TopMenuButtonWidth = 200; // In viewport space
const ui8  TopMenuTextHeight = NoteTextHeight;

const ui16 ColourPanelEdgeOffset = 25;
const ui8  ColourWheelVertices = 36;
const ui8  ColourPanelSliderWidth = 25;
const ui8  ColourPanelRGBBoxTextHeight = NoteTextHeight;
const ui8  ColourPanelRGBBoxOffset = NoteTextBorder;
const f32  ColourPanelWheelHeight = (ColourPanelRGBBoxTextHeight + ColourPanelRGBBoxOffset * 2) * 4 + ColourPanelRGBBoxOffset * 3;
const f32  ColourPanelFavouritesLayerHeight = ColourPanelWheelHeight / 3;
const f32  ColourPanelOKCancelTextHeight = NoteTextHeight;
const f32  ColourPanelOKCancelTextOffset = NoteTextBorder;
#define    ColourPanelButtonsHighlightRGBA 200, 200, 200, 0.5f

const ui8  UIBorderThickness = 2;

struct colour_panel_colour {
	vector  wheelSelection;
	f32     sliderCenterY;
};

program_unique struct {
	struct {
		colour_panel_colour colour;
		vector 							translation; // In viewport space, applied post scaling, therefore must be scaled
		f32    							scale = 1.0f;
	} 			 							canvas; // Treated as the GL projeciton matrix, initially takes up entire viewport, including title and tool bars
	
	text_body titleBar; // Coords in viewport space
	
	struct {
		rectangle background;
		button    save;
		button    load;
	} 					topMenu;
	
	struct {
		rectangle background; // In viewport space
		struct {
			rectangle 	background;
			const char* text;
			ui16        textBottom;
		} 						buttons[4];
		ui16          textHeight;
	} 							toolBar;
	
	struct { // All cords in UI viewport space
		bool display;
		
		colour_panel_colour currentColour;
		colour_panel_colour savedCurrentColour;
		bool      					updatingCurrentColour;
		bool      					updatingSlider;
		
		colour_panel_colour* colourBeingUpdated;
		
		rectangle frame;
		
		text_body red;
		text_body green;
		text_body blue;
		text_body hex; 
		text_body* bodyBeingUpdated; // RGB or hex
		
		colour_panel_colour favourites[6];
		ui8       					favouriteSelected; // 1 -> favourites array length
		
		button save;
		button ok;
		button cancel;
	} colourPanel;

	struct {
		memory_block memory;
		note* 			 selected;
		bool         moving;
		bool 				 justCreated;
	} 						 notes;
} state;

// Misc
#define GetTitleBar() (&state.titleBar)
#define GetToolBar() 	(&state.toolBar)
#define GetTopMenu() 	(&state.topMenu)
bool    MouseIsWithinToolbar(win32_state& osState);
bool    MouseOverlapsCanvas(win32_state& osState, rectangle& r);
bool    MouseOverlapsGUI(win32_state& osState, rectangle& r);
bool    TitleIsBeingUpdated();

// Colour panel
bool 			ColourPanelColourIsInited(colour_panel_colour& c);
bool 			ColourPanelIsVisible();
colour 	  ConvertColourPanelColourToRGB(colour_panel_colour& c);
#define 	GetColourPanel() (&state.colourPanel)
rectangle GetColourPanelSliderRectangle();
rectangle GetColourPanelWheelRectangle();
void 			OpenColourPanel(colour_panel_colour* colourToUpdate); // Can set colourToUpdate to Null
void 			UpdateColourPanelHex(colour_panel_colour colour);

// Space and projection matrix stuff
vector  ConvertToCanvasSpace(f32 x, f32 y);
vector  ConvertToCanvasSpace(vector pos);
vector  ConvertToViewportSpace(vector pos);
void 		SetGUIProjectionMatrix();
void 		SetCanvasProjetionMatrix();

#endif