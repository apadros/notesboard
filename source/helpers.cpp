#include <windows.h>
#include <gl\gl.h>
#include "apad_array.h"
#include "apad_error.h"
#include "apad_intrinsics.h"
#include "apad_maths.h"
#include "apad_memory.h"
#include "apad_opengl.h"
#include "apad_string.h"
#include "apad_win32_gui.h"
#include "helpers.h"

program_external note* CreateNote(vector pos, const char* title, const char* text) {
	if(TextIsBeingUpdated() == true)
		EndTextUpdate();

	note* n = Null;

	// Search for a free slot
	BeginNotesMemoryLoop(t) {
		if(NoteMemoryIsInUse(t) == false) {
			n = t;
			break;
		}
	}
	EndNotesMemoryLoop();

	// If not found, allocate more memory
	if(n == Null) {
		auto newBlock = AllocateMemory(state.notes.memory.size * 2);
		Copy(state.notes.memory.memory, state.notes.memory.size, newBlock.memory);
		Free(state.notes.memory);
		state.notes.memory = newBlock;
		n = (note*)((ui8*)state.notes.memory.memory + state.notes.memory.size / 2);
	}
	Assert(n != Null);

	// Text
	n->text = AllocateTextBody(pos.x, pos.y, NoteMinWidth, NoteTextBorder, NoteTextHeight, Null,
														 TextBodyFlagLetters | TextBodyFlagBulletPoints | TextBodyFlagNewlines | TextBodyFlagLeftAligned);
	if(text != Null) {
		Insert((char*)text, GetLength(text), n->text, 0);
		UpdateNoteContainers(n);
	}		

	// Title
	if(title != Null) {
		auto textRec = GetTextRectangle(n->text);
		n->title = AllocateTextBody(textRec.left, GetTopRight(textRec).y, textRec.width, NoteTextBorder, NoteTitleTextHeight, Null, TextBodyFlagLetters);
		Insert((char*)title, GetLength(title), n->title, 0);
		UpdateNoteContainers(n);
	}
	
	return n;
}

program_external void UpdateNoteContainers(note* n) {
	Assert(n != Null);
	
	// Update widths as needed
	f32 width = GetTextRectangle(n->text).width + n->text.textBorderOffset * 2;
	if(NoteHasTitle(n) == true)
		width = GetMax(width, GetTextRectangle(n->title).width + n->title.textBorderOffset * 2);
	if(width < NoteMinWidth)
		width = NoteMinWidth;
	n->text.container.width = width;
	if(NoteHasTitle(n) == true) {
		n->title.container.width = width;
		n->title.container.left = n->text.container.left;
	}
	
	// Update positions as needed in case of a change of text height
	auto heightPre = n->text.container.height;
	auto heightPost = GetTextRectangle(n->text).height + n->text.textBorderOffset * 2;
	if(heightPre != heightPost) {
		n->text.container.height = heightPost;
		n->text.container.bottom -= heightPost - heightPre;
	}
	if(NoteHasTitle(n) == true) // Regardless of updated text
		n->title.container.bottom = GetTopRight(n->text.container).y;
}

program_external bool NoteMemoryIsInUse(note* n) {
	Assert(n != Null);
	return IsValid(n->text);
}

program_external bool NoteHasTitle(note* n) {
	Assert(n != Null);
	return IsValid(n->title);
}

program_external rectangle GetColourPanelOuterWheelRectangle() {
	auto* panel = GetColourPanel();	
	f32 left = panel->frame.left + ColourPanelEdgeOffset;
	f32 top = GetTopRight(panel->frame).y - ColourPanelEdgeOffset;
	return CreateRectangle(left, top - ColourPanelWheelHeight, ColourPanelWheelHeight, ColourPanelWheelHeight);
}

program_external rectangle GetColourPanelInnerWheelRectangle() {
	auto outer = GetColourPanelOuterWheelRectangle();
	auto center = GetCenter(outer);
	f32  size = outer.height - ColourWheelThickness * 2 - ColourPanelRGBBoxOffset* 2;
	return CreateRectangle(center.x - size / 2, center.y - size / 2, size, size);
}

program_external rectangle GetNoteOverallRectangle(note* n) {
	Assert(n != Null);
	
	rectangle ret = n->text.container;
	if(NoteHasTitle(n) == true)
		ret.height += n->title.container.height;
	
	return ret;
}

program_external bool MouseIsWithinToolbar(win32_state& osState) {
	return Overlap(osState.mousePos.x, osState.mousePos.y, UnpackRectangle(state.toolBar.background));
}

program_external bool MouseOverlapsGUI(win32_state& osState, rectangle& r) {
	return Overlap(osState.mousePos.x, osState.mousePos.y, UnpackRectangle(r));
}

program_external bool MouseOverlapsCanvas(win32_state& osState, rectangle& r) {
	// Check first if within GUI space
	if(osState.mousePos.x <= GetTopRight(GetToolBar()->background).x || osState.mousePos.y >= GetTitleBar()->container.bottom)
		return false;
	
	auto pos = ConvertToCanvasSpace(osState.mousePos);
	return Overlap(pos.x, pos.y, UnpackRectangle(r));
}

program_external bool TitleIsBeingUpdated() {
	return TextIsBeingUpdated() == true && GetCurrentTextBody() == GetTitleBar();
}

program_external note* GetCurrentNote() {
	return state.notes.selected;
}

program_external note* SetCurrentNote(note* n) {
	return state.notes.selected = n;
}

program_external bool NoteTextIsBeingUpdated() {
	return TextIsBeingUpdated() == true && GetCurrentNote() != Null && GetCurrentTextBody() == &GetCurrentNote()->text;
}

program_external bool NoteTitleIsBeingUpdated() {
	return TextIsBeingUpdated() == true && GetCurrentNote() != Null && NoteHasTitle(GetCurrentNote()) == true && GetCurrentTextBody() == &GetCurrentNote()->title;
}

program_external bool NoteIsBeingUpdated() {
	return NoteTextIsBeingUpdated() == true || NoteTitleIsBeingUpdated() == true;
}

program_external vector ConvertToCanvasSpace(f32 x, f32 y) {
	vector p = {};
	p.x = (x - state.canvas.translation.x) / state.canvas.scale;
	p.y = (y - state.canvas.translation.y) / state.canvas.scale;
	return p;
}

program_external vector ConvertToCanvasSpace(vector pos) {
	return ConvertToCanvasSpace(pos.x, pos.y);
}

program_external vector ConvertToViewportSpace(vector pos) {
	vector p = {};
	p.x = pos.x * state.canvas.scale + state.canvas.translation.x;
	p.y = pos.y * state.canvas.scale + state.canvas.translation.y;
	return p;
}

program_external void SetGUIProjectionMatrix() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	AssertOpenGL();

	auto size = Win32GetProgramWindowClientSize();
	Assert(size.width > 0 && size.height > 0);
	glOrtho(0, size.width, 0, size.height, -1, 1);
	AssertOpenGL();
}

program_external void SetCanvasProjetionMatrix() {
	SetGUIProjectionMatrix();
	if(state.canvas.scale != 1.0f)
		glScalef(state.canvas.scale, state.canvas.scale, 1.0f);
	if(state.canvas.translation.x != 0 || state.canvas.translation.y != 0)
		glTranslatef(state.canvas.translation.x / state.canvas.scale, state.canvas.translation.y / state.canvas.scale, Null);
	AssertOpenGL();
}

program_external colour ConvertCurrentColourPanelColourToRGB() {
	auto* panel = GetColourPanel();
	
	// Work out the final colour based on the angle
	f32 wheelRed = 0;
	f32 wheelGreen = 0;
	f32 wheelBlue = 0;
	if(panel->outerWheel <= 120) {
		wheelRed = LERP(1.0f, 0, panel->outerWheel / 120);
		wheelGreen = LERP(0, 1.0f, panel->outerWheel / 120);
	}
	else if(panel->outerWheel <= 240) {
		wheelGreen = LERP(1.0f, 0, (panel->outerWheel - 120) / 120);
		wheelBlue = LERP(0, 1.0f, (panel->outerWheel - 120) / 120);
	}
	else {
		wheelRed = LERP(0, 1.0f, (panel->outerWheel - 240) / 120);
		wheelBlue = LERP(1.0f, 0, (panel->outerWheel - 240) / 120);
	}
	
	// @TODO - Technically, if the inner wheel is 120 <= x <= 240, there is no need to check the outer wheel. Optimise.
	f32 finalRed = 0.0f;
	f32 finalGreen = 0.0f;
	f32 finalBlue = 0.0f;
	if(panel->innerWheel <= 120) { // Between target colour and black
		finalRed = LERP(wheelRed, 0.0f, panel->innerWheel / 120);
		finalGreen = LERP(wheelGreen, 0.0f, panel->innerWheel / 120);
		finalBlue = LERP(wheelBlue, 0.0f, panel->innerWheel / 120);
	}
	else if(panel->innerWheel <= 240) { // Between black and white
		finalRed = LERP(0.0f, 1.0f, (panel->innerWheel - 120) / 120);
		finalGreen = LERP(0.0f, 1.0f, (panel->innerWheel - 120) / 120);
		finalBlue = LERP(0.0f, 1.0f, (panel->innerWheel - 120) / 120);
	}
	else { // Between target colour and white
		finalRed = LERP(1.0f, wheelRed, (panel->innerWheel - 240) / 120);
		finalGreen = LERP(1.0f, wheelGreen, (panel->innerWheel - 240) / 120);
		finalBlue = LERP(1.0f, wheelBlue, (panel->innerWheel - 240) / 120);
	}
	
	return CreateColourF32(finalRed, finalGreen, finalBlue);
}

#include <stdio.h> // For conversion to hex
program_external void UpdateColourPanelRGBHexText() {
	auto* panel = GetColourPanel();
	
	colour c = ConvertCurrentColourPanelColourToRGB();
	UpdateColourPanelRGBText(c.red.i, panel->red);
	UpdateColourPanelRGBText(c.green.i, panel->green);
	UpdateColourPanelRGBText(c.blue.i, panel->blue);
	
	// Update hex
	// Hex
	char buffer[7] = { '#' };
	sprintf(buffer + 1, "%02x", c.red.i);
	sprintf(buffer + 3, "%02x", c.green.i);
	sprintf(buffer + 5, "%02x", c.blue.i);

	ClearText(panel->hex);
	Insert(buffer, 7, panel->hex, 0);	
}

program_external void UpdateColourPanelRGBText(ui8 number, text_body& tb) {
	char* string = ToString(number);
	if(GetLength(string) == 1)
		string = Concatenate(2, "00", string);
	else if(GetLength(string) == 2)
		string = Concatenate(2, "0", string);			
	ClearText(tb);
	Insert(string, 3, tb, 0);
	Free(string);			
}

program_external void OpenColourPanel() {
	auto* panel = GetColourPanel();
			
	panel->display = true;
	
	#if 0 // @COLOUR_PANEL_REWORK
	panel->colourBeingUpdated = colourToUpdate;
	if(panel->colourBeingUpdated != Null)
		panel->savedCurrentColour = *colourToUpdate;
	#endif
	
	// Create the panel
	panel->frame.left = GetTopRight(GetToolBar()->background).x + 100;
	panel->frame.height = ColourPanelEdgeOffset + ColourPanelWheelHeight + ColourPanelEdgeOffset + ColourPanelFavouritesLayerHeight + ColourPanelEdgeOffset + ColourPanelOKCancelTextHeight + ColourPanelOKCancelTextOffset * 2 + ColourPanelEdgeOffset;
	panel->frame.bottom = GetCenter(GetToolBar()->buttons[3].background).y - panel->frame.height / 2;

	auto wheel = GetColourPanelOuterWheelRectangle();
	panel->outerWheel = panel->savedOuterWheelAngle;
	panel->innerWheel = panel->savedInnerWheelAngle;
		
	// RGB & hex boxes
	{
		f32 rgbBoxWidth = GetTextRenderSize("000", Null, ColourPanelRGBBoxTextHeight).width + ColourPanelRGBBoxOffset * 2;
		f32 left = GetTopRight(wheel).x + ColourPanelEdgeOffset;
		f32 height = ColourPanelRGBBoxTextHeight + ColourPanelRGBBoxOffset * 2;
		f32 offset = (wheel.height - height * 4) / 3;
		panel->red = AllocateTextBody(left, wheel.bottom + wheel.height - height, rgbBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, 3, Null);
		panel->green = AllocateTextBody(left, panel->red.container.bottom - offset - height, rgbBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, 3, Null);
		panel->blue = AllocateTextBody(left, panel->green.container.bottom - offset - height, rgbBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, 3, Null);
		panel->hex = AllocateTextBody(left, wheel.bottom, GetTextRenderSize("#000000", Null, ColourPanelRGBBoxTextHeight).width + ColourPanelRGBBoxOffset * 2, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, 7, TextBodyFlagLetters | TextBodyFlagLeftAligned);
		UpdateColourPanelRGBHexText();
	}
	
	panel->frame.width = GetTopRight(panel->green.container).x + ColourPanelEdgeOffset + GetTextRenderSize("Green", Null, panel->green.textHeight).width + ColourPanelEdgeOffset - (wheel.left - ColourPanelEdgeOffset);
		
	// Custom colour save button
	{
		f32 width = GetTextRenderSize("Save", 4, NoteTextHeight).width + NoteTextBorder * 2;
		f32 left = panel->frame.left + panel->frame.width - ColourPanelEdgeOffset - width;
		f32 height = NoteTextHeight + NoteTextBorder * 2;
		f32 centerY = GetColourPanelOuterWheelRectangle().bottom - ColourPanelEdgeOffset - ColourPanelFavouritesLayerHeight / 2;
		panel->save = AllocateButton(left, centerY - height / 2, width, height, "Save", NoteTextHeight, ColourPanelButtonsHighlightRGBA);
		panel->favouriteSelected = Null;
	}
	
	// Favourites
	{
		f32 start = panel->frame.left + ColourPanelEdgeOffset;
		f32 end = panel->save.rectangle.left - ColourPanelEdgeOffset;
		ui8 count = GetArrayLength(panel->favourites);
		f32 elementHeight = ColourPanelFavouritesLayerHeight / 2;
		auto* layouts = GetUIElementLayouts(start, end, count, 
																				elementHeight, elementHeight, elementHeight, elementHeight,
																				elementHeight, elementHeight, elementHeight, elementHeight);
		f32 centerY = GetColourPanelOuterWheelRectangle().bottom - ColourPanelEdgeOffset - ColourPanelFavouritesLayerHeight / 2;
		ForAll(count) {
			auto* f = panel->favourites + it;
			f->center = CreateVector(layouts[it].center, centerY);
			f->radius = elementHeight / 2;
		}
		FreeUIElementLayouts(layouts);
	}
	
	// Ok and cancel buttons
	{
		f32 width = GetTextRenderSize("Cancel", Null, NoteTextHeight).width + ColourPanelEdgeOffset * 2;
		f32 height = ColourPanelOKCancelTextHeight + ColourPanelOKCancelTextOffset * 2;
		f32 bottom = wheel.bottom - ColourPanelEdgeOffset - ColourPanelFavouritesLayerHeight - ColourPanelEdgeOffset - height;
		f32 offset = (panel->frame.width - width * 2) / 3;
		panel->ok = AllocateButton(panel->frame.left + offset, bottom, width, height, "OK", NoteTextHeight, ColourPanelButtonsHighlightRGBA);
		panel->cancel = AllocateButton(panel->frame.left + offset + width + offset, bottom, width, height, "Cancel", NoteTextHeight, ColourPanelButtonsHighlightRGBA);
	}
}

program_external void UpdateColourPanelWheels(ui8 red, ui8 green, ui8 blue) {
	colour c = CreateColourUI8(red, green, blue);
	auto* panel = GetColourPanel();
	
	if(c.red.i == c.green.i && c.green.i == c.blue.i) { // Black to white scale
		panel->outerWheel = 0;
		panel->innerWheel = 120 + c.red.f * 120;
	}
	else {
		// The lowest number will determine the position of the inner wheel towards pure white,
		// whereas the other 2 numbers will determine the inner wheel's position towards pure black
		ForAll(3) {
			f32 targetColours[] = { c.red.f, c.green.f, c.blue.f };
			f32 coloursAfterOnWheel[] = { c.green.f, c.blue.f, c.red.f };
			f32 coloursAfterAngle[] = { 120, 240, 0 };
			f32 lastColours[] = { c.blue.f, c.red.f, c.green.f };
			
			f32 targetColour = targetColours[it];
			f32 colourAfterOnWheel = coloursAfterOnWheel[it];
			f32 colourAfterAngle = coloursAfterAngle[it];
			f32 lastColour = lastColours[it];
			
			if(targetColour < colourAfterOnWheel && targetColour < lastColour) {
				if(targetColour == 0.0f) { // Scaling towards black
					Assert(colourAfterOnWheel + lastColour <= 1.0f);
					f32 scale = colourAfterOnWheel + lastColour;
					panel->innerWheel = (1.0f - scale) * 120;
					
					// Need to scale the other 2 channels back
					f32 perc = panel->innerWheel / 120;
					f32 colourAfter = colourAfterOnWheel / (1.0f - perc);
					panel->outerWheel = colourAfterAngle + (1.0f - colourAfter) * 120;
					
					break;
				}
				else { // Scaling towards white
					panel->innerWheel = 360 - targetColour * 120;
					
					// Need to scale the other 2 channels back
					f32 perc = targetColour;
					f32 colourAfter = (colourAfterOnWheel - perc) / (1.0f - perc);
					panel->outerWheel = colourAfterAngle + (1.0f - colourAfter) * 120;
					
					break;
				}
			}
		}
	}
}