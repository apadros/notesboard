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

program_external colour GetCurrentColourPanelColour() {
	auto* panel = GetColourPanel();
	
	colour outerWheelSelection = GetColourPanelOuterWheelColour(panel->outerWheel);
	return GetColourPanelInnerWheelColour(panel->innerWheel, outerWheelSelection);
}

#include <stdio.h> // For conversion to hex
program_external void SetColourPanelRGBHexText() {
	auto* panel = GetColourPanel();
	
	colour c = GetCurrentColourPanelColour();
	SetColourPanelRGBText(c.red.i, panel->red);
	SetColourPanelRGBText(c.green.i, panel->green);
	SetColourPanelRGBText(c.blue.i, panel->blue);
	
	// Update hex
	// Hex
	char buffer[7] = { '#' };
	sprintf(buffer + 1, "%02x", c.red.i);
	sprintf(buffer + 3, "%02x", c.green.i);
	sprintf(buffer + 5, "%02x", c.blue.i);

	ClearText(panel->hex);
	Insert(buffer, 7, panel->hex, 0);	
}

program_external void SetColourPanelRGBText(ui8 number, text_body& tb) {
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
		SetColourPanelRGBHexText();
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

program_external void SetColourPanelColour(ui8 red, ui8 green, ui8 blue) {
	colour c = CreateColourUI8(red, green, blue);
	auto* panel = GetColourPanel();
	
	if(c.red.i == c.green.i && c.green.i == c.blue.i) { // Black to white scale
		panel->outerWheel = 0;
		panel->innerWheel = 120 + c.red.f * 120;
	}
	else if(c.red.i == 255 && c.green.i == 0 && c.blue.i == 0) { // Pure red
		panel->outerWheel = 0;
		panel->innerWheel = 0;
	}
	else if(c.red.i == 0 && c.green.i == 255 && c.blue.i == 0) { // Pure green
		panel->outerWheel = 120;
		panel->innerWheel = 0;
	}
	else if(c.red.i == 0 && c.green.i == 0 && c.blue.i == 255) { // Pure blue
		panel->outerWheel = 240;
		panel->innerWheel = 0;
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
			
			f32 realColourAfter = 0;
			f32 realLastColour = 0;
			if(targetColour < colourAfterOnWheel && targetColour < lastColour) { // Lowest channel
				if(targetColour == 0.0f) { // Scaling towards black
					// The starting colour will always have a channel == 255,
					// otherwise the inner wheel angle != 0
					
					if(colourAfterOnWheel == 1.0f || lastColour == 1.0f) { // Inner wheel at angle == 0
						panel->innerWheel = 0;
						realColourAfter = colourAfterOnWheel;
						realLastColour = lastColour;
					}
					else { // Scale back first
						f32 beta = 1.0f - GetMax(colourAfterOnWheel, lastColour);
						panel->innerWheel = beta * 120;
						realColourAfter = ((beta * 0) - colourAfterOnWheel) / (beta - 1.0f);
						realLastColour = ((beta * 0) - lastColour) / (beta - 1.0f);
					}
				}
				else { // Scaling towards white
					panel->innerWheel = 360 - targetColour * 120;
					
					// Need to scale the other 2 channels back. Carry out inverse LERP
					f32 beta = targetColour;
					realColourAfter = ((beta * 1.0f) - colourAfterOnWheel) / (beta - 1.0f);
					realLastColour = ((beta * 1.0f) - lastColour) / (beta - 1.0f);
				}
				
				f32 angle = colourAfterAngle; // Because current target channel strength == 0
				if(realLastColour < 1.0f)
					angle = LERP(angle, angle + 60, realLastColour);
				else
					angle = LERP(angle + 60, angle + 120, 1.0f - realColourAfter);
				
				panel->outerWheel = angle;
				
				break;
			}
		}
	}
}

program_external colour GetColourPanelOuterWheelColour(f32 angle) {
	Assert(angle <= 360);
	// Each colour is at full strength at center +- 60, then LERPs off to 0 at other colours' centers
					
	// Work out percentages of each
	f32 red = 0;
	f32 green = 0;
	f32 blue = 0;
	if(angle <= 60) {
		red = 1.0f;
		green = LERP(0, 1.0f, angle / 60);
	}
	else if(angle <= 120) { // Pure bottom left colour center
		red = LERP(1.0f, 0, (angle - 60) / 60);
		green = 1.0f;
	}
	else if(angle <= 180) {
		green = 1.0f;
		blue = LERP(0, 1.0f, (angle - 120) / 60);
	}
	else if(angle <= 240) { // Pure bottom right colour center
		green = LERP(1.0f, 0, (angle - 180) / 60);
		blue = 1.0f;
	}
	else if(angle <= 300) {
		red = LERP(0, 1.0f, (angle - 240) / 60);
		blue = 1.0f;
	}
	else if(angle <= 360) { // Pure top colour center
		red = 1.0f;
		blue = LERP(1.0f, 0, (angle - 300) / 60);
	}
	
	return CreateColourF32(red, green, blue);
}

program_external colour GetColourPanelInnerWheelColour(f32 angle, colour top) {
	Assert(angle <= 360);
	
	// @TODO - Technically, if the inner wheel is 120 <= x <= 240, there is no need to check the outer wheel. Optimise.
	f32 finalRed = 0.0f;
	f32 finalGreen = 0.0f;
	f32 finalBlue = 0.0f;
	if(angle <= 120) { // Between target colour and black
		finalRed = LERP(top.red.f, 0.0f, angle / 120);
		finalGreen = LERP(top.green.f, 0.0f, angle / 120);
		finalBlue = LERP(top.blue.f, 0.0f, angle / 120);
	}
	else if(angle <= 240) { // Between black and white
		finalRed = LERP(0.0f, 1.0f, (angle - 120) / 120);
		finalGreen = LERP(0.0f, 1.0f, (angle - 120) / 120);
		finalBlue = LERP(0.0f, 1.0f, (angle - 120) / 120);
	}
	else { // Between target colour and white
		finalRed = LERP(1.0f, top.red.f, (angle - 240) / 120);
		finalGreen = LERP(1.0f, top.green.f, (angle - 240) / 120);
		finalBlue = LERP(1.0f, top.blue.f, (angle - 240) / 120);
	}
	
	return CreateColourF32(finalRed, finalGreen, finalBlue);
}