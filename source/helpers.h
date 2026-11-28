#ifndef TEXT_H
#define TEXT_H

#include "apad_base_types.h"
#include "apad_gui.h"
#include "apad_intrinsics.h."
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_string_args.h"
#include "apad_time.h"
#include "apad_win32_gui.h"

// ******************** Boards ******************** //

const ui8 BoardIconSize = 100;

struct board {
	vector      pos;   // Within previous board canvas
	text_body 	title; // Applicable only within previous canvas. Container pos & size need to be checked separately from board.pos
	
	memory_pool notes;  // Allocated during create of first note
	memory_pool boards; // Allocated during create of first board
	
	struct {
		colour colour;
		vector translation; // In viewport space, applied post scaling, therefore must be scaled
		f32    scale;
	} 			 canvas; // Treated as the GL projection matrix, initially takes up entire viewport, including title and tool bars
};

#define BeginBoardsMemoryLoop(_allocatedBoolPtrID, _folderID) BeginMemoryPoolLoop(GetCurrentBoard()->folders, _allocatedBoolPtrID, _memPtrID) \
																														     board* _folderID = (board*)_memPtrID;
#define BreakBoardsMemoryLoop()													     	break
#define EndBoardsMemoryLoop() 													      EndMemoryPoolLoop()

board* CreateBoard(vector pos, char* title);
char* 	// Needs to be freed with Free(char*) if not Null
				GetBoardTitle(board* b);
board* MouseOverlapsBoard(win32_state& osState); // Returns folder if mouse overlaps the folder itself or the text. Will return Null if none.

void CloseCurrentBoard();
void OpenBoard(board* b);

// ******************** Notes ******************** //

const ui16 NoteMinWidth = 200;
const ui16 NoteTextHeight = 15;
const ui16 NoteTextBorderOffset = NoteTextHeight;
const ui16 NoteMinHeight = NoteTextHeight + NoteTextBorderOffset * 2;

const ui16 NoteTitleTextHeight = NoteTextHeight * 1.5f;

struct note {
	text_body title; // Edges in canvas space
	text_body text;  // Edges in canvas space
									 // Call GetTextLength(note->text) (APAD API) for the real length
};

#define   BeginNotesMemoryLoop(_allocatedBoolPtrID, _notePtrID) BeginMemoryPoolLoop(GetCurrentBoard()->notes, _allocatedBoolPtrID, _memPtrID) { \
																														      note* _notePtrID = (note*)_memPtrID;
#define   EndNotesMemoryLoop() 				                          } EndMemoryPoolLoop()

note*     CreateNote(vector pos, char* title, char* text);
void 			DeleteNote(note* n);
note*     GetCurrentNote(); // Will return Null if none
note* 	  SetCurrentNote( // Will set programState.state to NoteSelected
													note* n); // Set n to Null to deselect current note, will set programState.state to Idle

rectangle GetNoteOverallRectangle(note* n);
bool 			NoteIsBeingUpdated();
bool 		  NoteTextIsBeingUpdated();
bool 			NoteTitleIsBeingUpdated();
bool 		  NoteHasTitle(note* n);
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
const ui8  ColourWheelVertices = 64;
const f32  ColourWheelThickness = ColourPanelEdgeOffset;
const ui8  ColourPanelSliderWidth = 25;
const ui8  ColourPanelRGBBoxTextHeight = NoteTextHeight;
const ui8  ColourPanelRGBBoxOffset = NoteTextBorderOffset;
const f32  ColourPanelWheelHeight = (ColourPanelRGBBoxTextHeight + ColourPanelRGBBoxOffset * 2) * 4 + ColourPanelRGBBoxOffset * 3;
const f32  ColourPanelFavouritesLayerHeight = ColourPanelWheelHeight / 3;
const f32  ColourPanelOKCancelTextHeight = NoteTextHeight;
const f32  ColourPanelOKCancelTextOffset = NoteTextBorderOffset;
#define    ColourPanelButtonsHighlightRGBA 200, 200, 200, 0.5f

const ui8  UIBorderThickness = 2;

enum struct program_state : ui8 { 
	Idle, 
	BoardCreated, // Implies also being moved until left mouse button up
	BoardSelected, BoardMoving, 
	NoteCreated, // Implies also being moved until left mouse button up 
	NoteSelected, NoteMoving 
};

program_unique struct {
	struct {
		rectangle background; // In viewport space
		button    save;
		button    load;
	} 					topMenu;
	
	struct {
		rectangle background; // In viewport space
		button    newNote;
		button    bulletPoint;
		button    noteTitle;
		button    colourPanel;
		button    CreateBoard;
	} 					toolBar;
	
	struct { // All cords in UI viewport space
		bool display;
		
		f32  	 mainColour; // Angle
		vector shade; 		 // Percentage from bottom left corner
		bool 	 updatingMainColour;
		bool 	 updatingShade;
		f32    savedMainColour;
		vector savedShade; // 0 -> 1
		
		rectangle frame;
		
		text_body  red;
		text_body  green;
		text_body  blue;
		text_body  hex; 
		text_body* bodyBeingUpdated; // RGB or hex
		
		button redArrowUp;
		button redArrowDown;
		button greenArrowUp;
		button greenArrowDown;
		button blueArrowUp;
		button blueArrowDown;
		
		struct {
			colour colour;
			vector center;
			f32    radius;
			bool   inited;
		} 			 favourites[8];
		ui8    	 favouriteSelected; // 1 -> favourites array length
		
		button save;
		button ok;
		button cancel;
	} colourPanel;
	
	program_state state;
	button	      backButton;
	memory_offset currentSelection;
	memory_stack  directories; // Keep track of boards opened. Will always have at least 1.
} programState;

// Misc
board* 	GetCurrentBoard();
#define GetToolBar() 	(&programState.toolBar)
#define GetTopMenu() 	(&programState.topMenu)
bool 		MouseIsWithinCanvasSpace(win32_state& osState);
bool    MouseIsWithinToolbar(win32_state& osState);
bool    MouseOverlapsCanvas(win32_state& osState, rectangle& r);
bool    MouseOverlapsGUI(win32_state& osState, rectangle& r);

// Title bar
rectangle  GetTitleRectangle();
char* 	   // Needs to be freed with Free(char*)
					 GetTitleText();
text_body& GetTitleTextBody();
bool       TitleIsBeingUpdated();
void 		   UpdateTitleText(string_args(text)); // Will update title bar and current board title

// Colour panel
colour 	  GetCurrentColourPanelColour();
#define 	GetColourPanel() (&programState.colourPanel)
colour 		GetColourPanelWheelColour(f32 angle);
rectangle GetColourPanelWheelRectangle();
rectangle GetColourPanelRectangleRectangle();
colour 		GetColourPanelRectangleColour(f32 angle, colour top);
void 			OpenColourPanel();
void 			SetColourPanelColour(ui8 red, ui8 green, ui8 blue);
void 			SetColourPanelHexText();
void 			SetColourPanelRGBText(ui8 number, text_body& tb);
void 			SetColourPanelRGBHexText();

// Space and projection matrix stuff
vector  ConvertToCanvasSpace(f32 x, f32 y);
vector  ConvertToCanvasSpace(vector pos);
vector  ConvertToViewportSpace(vector pos);
void 		SetGUIProjectionMatrix();
void 		SetCanvasProjetionMatrix();

#endif