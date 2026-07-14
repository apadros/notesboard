#ifndef APAD_GUI_H
#define APAD_GUI_H

#include "apad_base_types.h"
#include "apad_memory.h"

// ******************** Text body ******************** //

// Use these in bitwise operations with text_body.flags
const ui8 TextBodyFlagLetters = 		 1;
const ui8 TextBodyFlagBulletPoints = 1 << 1;
const ui8 TextBodyFlagNewlines = 		 1 << 2;
const ui8 TextBodyFlagLeftAligned =  1 << 3; // If not present text is assumed to be center-aligned

struct text_body {
	memory_stack memory;
	rectangle    container;
	f32          textBorderOffset;
	f32          textHeight;
	ui8          flags;
};

dll_import text_body AllocateTextBody(f32 left, f32 bottom, f32 width, f32 textBorderOffset, f32 textHeight, ui8 flags);
dll_import void 		 ClearText(text_body& tb);
dll_import void 		 FreeText(text_body& tb);
dll_import char* 		 FindChar(char c, ui16 pos, bool scanForward, text_body& tb);
dll_import char* 		 GetText(text_body& tb);
dll_import ui32 		 GetTextLength(text_body& tb);
dll_export rectangle GetTextRectangle(text_body& tb); // Will have a min height of tb.textHeight, width will be 0 if it doesn't contain any text																 
dll_import ui16      Insert(char* string, ui32 length, text_body& tb, ui32 pos); // Returns chars inserted
dll_import bool 		 IsValid(text_body& tb);
dll_import void 		 RemoveChar(text_body& tb, ui32 pos);
dll_import void 		 Render(text_body& tb);

// ******************** Text update ******************** //

const ui8 BulletPointChar = '\b';
const ui8 NewlineChar = 		'\n';

dll_import void  BeginTextUpdate(text_body& text); // Will place the cursor at the end of the text body
dll_import void  EndTextUpdate();
dll_import f32 	 GetTextLineHeight(f32 textHeight);
dll_import bool  TextIsBeingUpdated();

// The following functions are only valid if text is being updated
struct win32_state;
struct text_update_pipeline_data {
	bool wantToLeaveTextBodyUp; 	// When pressing up at the top edge of a text_body
	bool wantToLeaveTextBodyDown; // When pressing down at the bottom edge of a text_body
};
dll_import ui16  										 GetCharOffsetFromStart(char* c);
dll_export text_body* 							 GetCurrentTextBody();
dll_export ui16       							 GetCursorCharOffset();
dll_import void  										 InsertCharAtCursor(char c);
dll_import text_update_pipeline_data RunTextUpdatePipeline(win32_state& osState);

// ******************** Cursor ******************** //

dll_export f32    GetCursorAlphaValue();
dll_import vector GetCursorPos(); // Will be relative to the bottom-left of the current text body
dll_import void   MoveCursor(si8 charOffset); // Current offset clamped between 0 and current text_body length
dll_export void   SetCursorCharOffset(ui16 offset); // Offset clamped to current text_body length

dll_export void 	_SetCursorPos(f32 x, f32 y); // Coords are relative to text_body origin, will be clamped to within its boundaries.
#define           SetCursorPos _SetCursorPos // Windows already has a SetCursorPos function

// ******************** Rendering ******************** //

dll_import void      DrawRectangleBorder(f32 left, f32 bottom, f32 width, f32 height, 
																				 f32 lineWidth, 
																				 ui8 r, ui8 g, ui8 b);
dll_import void      DrawCircleBorder(f32 centerX, f32 centerY, f32 radius, 
																			ui8 lineWidth, 
																			ui8 r, ui8 g, ui8 b);
dll_import void      DrawRectangleFull(f32 left, f32 bottom, f32 width, f32 height, 
																			 ui8 r, ui8 g, ui8 b);
dll_import vector    GetTextRenderDimensions( // Will return a minimum y of height even if no text present, but x will equal 0
									   											   char* text, 
																						 ui32 length, // Set to Null to read the entire string
																						 f32 height); 
dll_import rectangle RenderText(char* text, ui32 length, 
																f32 	x, f32 y, f32 height, 
																bool  center);
dll_import f32       UI8ColourToF32(ui8 u);

#endif