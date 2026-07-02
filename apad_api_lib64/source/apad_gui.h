#ifndef APAD_GUI_H
#define APAD_GUI_H

#include "apad_base_types.h"
#include "apad_memory.h"

// ******************** Text body ******************** //

const ui8 TextBodyFlagLetters = 		 1;
const ui8 TextBodyFlagBulletPoints = 1 << 1;
const ui8 TextBodyFlagTabs = 				 1 << 2;
const ui8 TextBodyFlagNewlines = 		 1 << 3;
const ui8 TextBodyFlagLeftAligned =  1 << 4; // If not present text is assumed to be center-aligned

struct text_body {
	memory_stack memory;
	f32          textHeight;
	ui8          flags;
};

dll_import text_body AllocateTextBody(f32 textHeight, ui8 flags);
dll_import void 		 ClearTextBody(text_body& tb);
dll_import void 		 FreeTextBody(text_body& tb);
dll_import ui32 		 GetTextBodyLength(text_body& tb);
dll_import char* 		 GetTextBodyText(text_body& tb);
dll_import void 		 InsertString(char* string, ui32 length, text_body& tb, ui32 pos);
dll_import void 		 RemoveChar(text_body& tb, ui32 pos);
dll_import bool 		 TextBodyIsValid(text_body& tb);

// ******************** Text update ******************** //

dll_import bool TextIsBeingUpdated();
dll_import void InsertCharAtCursor(char c);
dll_import void EndTextUpdate();
dll_import void BeginTextUpdate(text_body& text); // Will place the cursor at the end of the text body
dll_import ui16 GetCharOffsetFromStart(char* c); // Only valid is text is being updated
dll_import char* FindChar(char c, ui16 pos, bool scanForward); // Only valid is text is being updated

// ******************** Cursor ******************** //

dll_import void MoveCursor(si8 charOffset);
dll_import void SetCursorPos(f32 x, f32 y);

// ******************** Rendering ******************** //

dll_import void      DrawRectangleBorder(f32 left, f32 bottom, f32 width, f32 height, f32 lineWidth, ui8 r, ui8 g, ui8 b);
dll_import void      DrawCircleBorder(f32 centerX, f32 centerY, f32 radius, ui8 lineWidth, ui8 r, ui8 g, ui8 b);
dll_import void      DrawRectangleFull(f32 left, f32 bottom, f32 width, f32 height, ui8 r, ui8 g, ui8 b);
dll_import vector    GetTextRenderDimensions( // Will return a minimum y of height even if no text present, but x will equal 0
									   											   char* text, ui32 length, f32 height); 
dll_import rectangle RenderText(char* text, 
																ui32  length, 
																f32 	x, 
																f32 	y, 
																f32 	height, 
																bool  center);
dll_import f32        UI8ColourToF32(ui8 u);

#endif