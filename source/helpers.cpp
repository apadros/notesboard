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