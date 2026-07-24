#include <windows.h>
#include <gl\gl.h>
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
	n->text = AllocateTextBody(pos.x, pos.y, NoteMinWidth, NoteTextBorder, NoteTextHeight,
														 TextBodyFlagLetters | TextBodyFlagBulletPoints | TextBodyFlagNewlines | TextBodyFlagLeftAligned);
	if(text != Null) {
		Insert((char*)text, GetLength(text), n->text, 0);
		UpdateNoteContainers(n);
	}		

	// Title
	if(title != Null) {
		auto textRec = GetTextRectangle(n->text);
		n->title = AllocateTextBody(textRec.left, GetTopRight(textRec).y, textRec.width, NoteTextBorder, NoteTitleTextHeight, TextBodyFlagLetters);
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

program_external rectangle GetColourPanelWheelRectangle() {
	auto* panel = GetColourPanel();	
	f32 left = panel->frame.left + ColourPanelEdgeOffset;
	f32 top = GetTopRight(panel->frame).y - ColourPanelEdgeOffset;
	return CreateRectangle(left, top - ColourPanelWheelHeight, ColourPanelWheelHeight, ColourPanelWheelHeight);
}

program_external rectangle GetColourPanelSliderRectangle() {
	auto* panel = GetColourPanel();
	auto  wheel = GetColourPanelWheelRectangle();
	f32 left = GetTopRight(wheel).x + ColourPanelEdgeOffset;
	return CreateRectangle(left, wheel.bottom, ColourPanelSliderWidth, wheel.height);
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

program_external bool ColourPanelIsVisible() {
	return GetColourPanel()->display;
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

program_external bool ColourPanelColourIsInited(colour_panel_colour& c) {
	return c.wheelSelection.x != Null && c.wheelSelection.y != Null && c.sliderCenterY != Null;
}

program_external colour ConvertColourPanelColourToRGB(colour_panel_colour& c) {
	if(ColourPanelColourIsInited(c) == false)
		return CreateColour(255, 255, 255);
	
	auto wheelRec = GetColourPanelWheelRectangle();

	vector vec = c.wheelSelection - GetCenter(wheelRec);
	
	// Scale magnitude
	f32 magnitude01 = Magnitude(vec) / (wheelRec.width / 2); // 0 -> 1 between circle center and outer edges

	// Angle of current selection
	f32 angle = 0; // About the horizontal axis
	if(vec.x == 0)
		angle = vec.y > 0 ? 90 : 270;
	else if(vec.y == 0)
		angle = vec.x > 0 ? 0 : 180;
	else {
		f32 a = Magnitude(vec.x);
		f32 o = Magnitude(vec.y);
		angle = ArcTan(o / a);
		if(vec.x < 0 && vec.y > 0)
			angle = 180 - angle;
		else if(vec.x < 0 && vec.y < 0)
			angle += 180;
		else if(vec.x > 0 && vec.y < 0)
			angle = 360 - angle;
	}
	
	// Adjust angle to start from the vertical axis (red)
	angle -= 90;
	if(angle < 0)
		angle += 360;

	// Work out the max colour based on the angle
	f32 rmax = 0;
	f32 gmax = 0;
	f32 bmax = 0;
	if(angle <= 120) {
		rmax = LERP(1.0f, 0, angle / 120);
		gmax = LERP(0, 1.0f, angle / 120);
	}
	else if(angle <= 240) {
		gmax = LERP(1.0f, 0, (angle - 120) / 120);
		bmax = LERP(0, 1.0f, (angle - 120) / 120);
	}
	else {
		rmax = LERP(0, 1.0f, (angle - 240) / 120);
		bmax = LERP(1.0f, 0, (angle - 240) / 120);
	}

	// Final wheel colour
	f32 wheelRed = LERP(1.0f, rmax, magnitude01);
	f32 wheelGreen = LERP(1.0f, gmax, magnitude01);
	f32 wheelBlue = LERP(1.0f, bmax, magnitude01);
	
	// Final colour taking the slider position into consideration
	auto sliderRec = GetColourPanelSliderRectangle();
	f32 sliderScale = (c.sliderCenterY - sliderRec.bottom) / sliderRec.height;
	f32 finalRed = wheelRed * sliderScale;
	f32 finalGreen = wheelGreen * sliderScale;
	f32 finalBlue = wheelBlue * sliderScale;
	
	colour ret = {};
	ret.red = finalRed;
	ret.green = finalGreen;
	ret.blue = finalBlue;
	return ret;
}

program_external void OpenColourPanel() {
	auto* panel = GetColourPanel();
			
	panel->display = true;
	
	// Create the panel
	panel->frame.left = GetTopRight(GetToolBar()->background).x + 100;
	panel->frame.height = ColourPanelEdgeOffset + ColourPanelWheelHeight + ColourPanelEdgeOffset + ColourPanelFavouritesLayerHeight + ColourPanelEdgeOffset + ColourPanelOKCancelTextHeight + ColourPanelOKCancelTextOffset * 2 + ColourPanelEdgeOffset;
	panel->frame.bottom = GetCenter(GetToolBar()->buttons[3].background).y - panel->frame.height / 2;

	auto wheel = GetColourPanelWheelRectangle();
	auto slider = GetColourPanelSliderRectangle();
	if(ColourPanelColourIsInited(panel->savedCurrentColour) == true) // If we have stored a current colour
		panel->currentColour = panel->savedCurrentColour;
	else {
		panel->currentColour.wheelSelection = GetCenter(wheel);
		panel->currentColour.sliderCenterY = GetTopRight(slider).y;
	}
	
	// RGB & hex boxes
	{
		f32  rgbBoxWidth = GetTextRenderSize("000", Null, ColourPanelRGBBoxTextHeight).width + ColourPanelRGBBoxOffset * 2;
		f32  left = GetTopRight(slider).x + ColourPanelEdgeOffset;
		f32  height = ColourPanelRGBBoxTextHeight + ColourPanelRGBBoxOffset * 2;
		f32  offset = (wheel.height - height * 4) / 3;
		panel->red = AllocateTextBody(left, wheel.bottom + wheel.height - height, rgbBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
		panel->green = AllocateTextBody(left, panel->red.container.bottom - offset - height, rgbBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
		panel->blue = AllocateTextBody(left, panel->green.container.bottom - offset - height, rgbBoxWidth, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, Null);
		panel->hex = AllocateTextBody(left, wheel.bottom, GetTextRenderSize("#000000", Null, ColourPanelRGBBoxTextHeight).width + ColourPanelRGBBoxOffset * 2, ColourPanelRGBBoxOffset, ColourPanelRGBBoxTextHeight, TextBodyFlagLetters | TextBodyFlagLeftAligned);
		Insert("#FFFFFF", 7, panel->hex, 0);
	}
	
	panel->frame.width = GetTopRight(panel->green.container).x + ColourPanelEdgeOffset + GetTextRenderSize("Green", Null, panel->green.textHeight).width + ColourPanelEdgeOffset - (wheel.left - ColourPanelEdgeOffset);
		
	// Custom colour save button
	{
		f32 width = GetTextRenderSize("Save", 4, NoteTextHeight).width + NoteTextBorder * 2;
		f32 left = panel->frame.left + panel->frame.width - ColourPanelEdgeOffset - width;
		f32 height = NoteTextHeight + NoteTextBorder * 2;
		f32 centerY = GetColourPanelWheelRectangle().bottom - ColourPanelEdgeOffset - ColourPanelFavouritesLayerHeight / 2;
		panel->save = AllocateButton(left, centerY - height / 2, width, height, "Save", NoteTextHeight, ColourPanelButtonsHighlightRGBA);
		panel->favouriteSelected = Null;
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