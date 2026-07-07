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
		CopyMemory(state.notes.memory.memory, state.notes.memory.size, newBlock.memory);
		FreeMemory(state.notes.memory);
		state.notes.memory = newBlock;
		n = (note*)((ui8*)state.notes.memory.memory + state.notes.memory.size / 2);
	}
	Assert(n != Null);

	// Text
	n->text = AllocateTextBody(NoteTextHeight,
														 TextBodyFlagLetters | TextBodyFlagBulletPoints | TextBodyFlagNewlines | TextBodyFlagLeftAligned);
	if(text != Null)
		InsertString((char*)text, GetStringLength(text), n->text, 0);

	// Title
	if(title != Null) {
		n->title = AllocateTextBody(NoteTitleTextHeight, TextBodyFlagLetters);
		InsertString((char*)title, GetStringLength(title), n->title, 0);
	}
	
	n->pos = pos;
	
	return n;
}

program_external bool NoteMemoryIsInUse(note* n) {
	Assert(n != Null);
	return TextBodyIsValid(n->text);
}

program_external bool NoteHasTitle(note* n) {
	Assert(n != Null);
	return TextBodyIsValid(n->title);
}

program_external rectangle GetColourPanelWheelRectangle() {
	auto* panel = GetColourPanel();
	f32 middleX = panel->frame.left + panel->frame.width / 3;
	f32 middleY = panel->frame.bottom + panel->frame.height * ColourWheelVerticalCenterMult;
	f32 size = ColourWheelSizeMult * panel->frame.width;
	return CreateRectangle(middleX - size / 2, middleY - size / 2, size, size);
}

program_external rectangle GetColourPanelSliderRectangle() {
	auto* panel = GetColourPanel();
	f32 middleX = panel->frame.left + panel->frame.width * 0.75f;
	f32 middleY = GetCenter(GetColourPanelWheelRectangle()).y;
	f32 width = 25;
	f32 height = ColourWheelSizeMult * panel->frame.width;
	return CreateRectangle(middleX - width / 2, middleY - height / 2, width, height);
}

program_external note_text_vectors GetNoteRectangles(note* n) {
	Assert(n != Null);
	
	// Text
	rectangle textContainer = CreateRectangle(n->pos, vector());
	rectangle textEdges = CreateRectangle(textContainer.pos + CreateVector(NoteTextBorder, NoteTextBorder), GetTextBodyRenderDimensions(n->text));
	textContainer.size = textEdges.size + CreateVector(NoteTextBorder * 2, NoteTextBorder * 2);
	
	// Title
	rectangle titleContainer = {};
	rectangle titleEdges = {};
	if(NoteHasTitle(n) == true) {
		titleEdges.size = GetTextBodyRenderDimensions(n->title);
		titleContainer = CreateRectangle(textContainer.left, textContainer.bottom + textContainer.height, 
																		 GetMax(titleEdges.width + NoteTextBorder * 2, textContainer.width), titleEdges.height + NoteTextBorder * 2);
		titleEdges.bottom = titleContainer.bottom + NoteTextBorder;
		titleEdges.left = GetCenter(titleContainer).x - titleEdges.width / 2;
	}
	if(titleContainer.width > textContainer.width)
		textContainer.width = titleContainer.width;
	
	rectangle overall = CreateRectangle(textContainer.left, textContainer.bottom, textContainer.width, textContainer.height + titleContainer.height);
	
	note_text_vectors ret = {};
	ret.titleEdges = titleEdges;
	ret.titleContainer = titleContainer;
	ret.textEdges = textEdges;
	ret.textContainer = textContainer;
	ret.overall = overall;
	return ret;
}

program_external bool MouseLeftDownThisFrame() { // @TODO - Export to APAD_API apad_win32_gui.cpp, add to os state struct 
	return state.mouse.lastLeftDown == false && state.mouse.leftDown == true;
}

program_external bool MouseIsWithinToolbar() {
	return Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackRectangle(state.toolBar.background));
}

program_external bool MouseOverlapsGUI(rectangle& r) {
	return Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackRectangle(r));
}

program_external bool MouseOverlapsCanvas(rectangle& r) {
	auto pos = ConvertToCanvasSpace(state.mouse.pos);
	return Overlap(pos.x, pos.y, UnpackRectangle(r));
}

program_external bool TitleIsBeingUpdated() {
	return TextIsBeingUpdated() == true && GetCurrentTextBody() == &state.titleBar.text;
}

program_external note* GetCurrentNote() {
	return state.notes.selected;
}

program_external note* SetCurrentNote(note* n) {
	return state.notes.selected = n;
}

program_external bool NoteIsBeingUpdated() {
	return TextIsBeingUpdated() == true && state.notes.selected != Null && GetCurrentTextBody() == &state.notes.selected->text;
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