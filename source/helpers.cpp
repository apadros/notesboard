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

program_external void SetCursorPos(f32 x, f32 y) {
	state.textUpdate.cursorPos.x = x;
	state.textUpdate.cursorPos.y = y;
}

program_external note* CreateNote(vector pos, const char* title, const char* text) {
	if(TextIsBeingWritten() == true)
				EndWriting();

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
	n->text = AllocateTextBody(pos.x, pos.y,
														 NoteTextHeight,
														 Null,
														 NoteTextBorderOffset
														 TextBodyFlagLetters | TextBodyFlagBulletPoints | TextBodyFlagTabs | TextBodyFlagNewlines | TextBodyFlagLeftAligned);
	if(text != Null)
		InsertString((char*)text, GetStringLength(text), n->text, 0);
	n->edges.width = GetMin(NoteMinWidth, n->text.edges.width);

	// Title
	if(title != Null) {
		n->title = AllocateTextBody(n->pos.x, n->pos.y + /* @TODO */, Null,
																NoteTitleTextHeight, Null, NoteTextBorder, TextBodyFlagLetters);
		InsertString((char*)title, GetStringLength(title), n->title, 0);
		n->text.boxWidth = GetMax(n->text.boxWidth, n->title.boxWidth);
		n->title.boxWidth = n->text.boxWidth;
	}

	return n;
}

program_external rectangle& GetTextBodyBackground(text_body& tb) {
	return tb.background;
}

program_external void BeginWriting(text_body& text) {
	auto* tu = &state.textUpdate;
	tu->textBody = &text;
	tu->cursorCharOffset = GetTextLength(text);
	tu->cursorBlinkTime = 0;
}

program_external bool NoteMemoryIsInUse(note* n) {
	Assert(n != Null);
	return n->background.width != 0 && n->background.height != 0;
}

program_external bool NoteHasTitle(note* n) {
	Assert(n != Null);
	return TextBodyIsValid(n->title);
}

program_external void DrawRectangleBorder(f32 left, f32 bottom, f32 width, f32 height, ui8 r, ui8 g, ui8 b) {
	glLineWidth(2);
	glBegin(GL_LINES);
	glColor3f(UI8ColourToF32(r), UI8ColourToF32(g), UI8ColourToF32(b));

	glVertex2f(left, bottom);
	glVertex2f(left, bottom + height);

	glVertex2f(left, bottom + height);
	glVertex2f(left + width, bottom + height);

	glVertex2f(left + width, bottom + height);
	glVertex2f(left + width, bottom);

	glVertex2f(left + width, bottom);
	glVertex2f(left, bottom);
	glEnd();
	AssertOpenGL();
}

program_external void DrawCircleBorder(f32 centerX, f32 centerY, f32 radius, ui8 lineWidth, ui8 r, ui8 g, ui8 b) {
	glLineWidth(lineWidth);
	glBegin(GL_LINE_LOOP);
	glColor3f(UI8ColourToF32(r), UI8ColourToF32(g), UI8ColourToF32(b));
	ui8 vertices = 72;
	FromToInc(0, vertices + 1) {
		f32 angle = it * 360 / vertices;
		f32 x = centerX - Sine(angle) * radius;
		f32 y = centerY + Cos(angle) * radius;
		glVertex2f(x, y);
	}
	glEnd();
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
	f32 middleY = GetMiddle(GetColourPanelWheelRectangle()).y;
	f32 width = 25;
	f32 height = ColourWheelSizeMult * panel->frame.width;
	return CreateRectangle(middleX - width / 2, middleY - height / 2, width, height);
}

program_external note_text_render_data GetNoteTextRenderData(note* n) {
	Assert(n != Null);

	note_text_render_data ret = {};

	// Title
	f32 textBodyTop = Null;
	if(NoteHasTitle(n) == true) {
		ret.title.height = NoteTitleTextHeight;
		ret.title.bottom = n->background.bottom + n->background.height - NoteTextBorder - ret.title.height;
		if(GetTextLength(n->title) > 0) {
			ret.title.width = GetTextRenderDimensions(GetTextStart(n->title), GetTextLength(n->title), NoteTitleTextHeight).x;
			ret.title.left = GetMiddle(n->background).x - ret.title.width / 2;
		}
		else {
			ret.title.left = GetMiddle(n->background).x;
			ret.title.width = 0;
		}
		textBodyTop = n->background.bottom + n->background.height - NoteTextBorder - ret.title.height - NoteTextBorder * 2;

		ret.titleContainer.left = n->background.left + NoteTextBorder;
		ret.titleContainer.width = GetMax(ret.title.width, n->background.width - NoteTextBorder * 2);
		ret.titleContainer.bottom = ret.title.bottom;
		ret.titleContainer.height = ret.title.height;
	}
	else
		textBodyTop = n->background.bottom + n->background.height - NoteTextBorder;

	// Text
	ret.text.left = n->background.left + NoteTextBorder;
	if(GetTextLength(n->text) == 0) {
		ret.text.width = 0;
		ret.text.height = NoteTextHeight;
		ret.text.bottom = textBodyTop - ret.text.height;
	}
	else {
		auto textDimensions = GetTextRenderDimensions(GetTextStart(n->text), GetTextLength(n->text), NoteTextHeight);
		Assert(textDimensions.x > 0);
		Assert(textDimensions.y >= NoteTextHeight);
		ret.text.width = textDimensions.x;
		ret.text.height = textDimensions.height;
		ret.text.bottom = textBodyTop - ret.text.height;
	}
	ret.textContainer.left = ret.text.left;
	ret.textContainer.width = GetMax(n->background.width - NoteTextBorder * 2, ret.text.width);
	ret.textContainer.bottom = ret.text.bottom;
	ret.textContainer.height = ret.text.height;

	return ret;
}

program_external bool MouseLeftDownThisFrame() {
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
	return TextIsBeingWritten() == true && state.textUpdate.textBody->memory.memory == &state.titleBar.text.memory.memory;
}

program_external void MoveCursor(si8 offset) {
	Assert(TextIsBeingWritten() == true);

	auto* tu = &state.textUpdate;
	auto  textLength = GetTextLength(*tu->textBody);
	Assert(tu->cursorCharOffset <= textLength);

	if(offset < 0) {
		if(-offset >= tu->cursorCharOffset)
			tu->cursorCharOffset = 0;
		else
			tu->cursorCharOffset += offset;
	}
	else if(offset > 0) {
		if(tu->cursorCharOffset + offset >= textLength)
			tu->cursorCharOffset = textLength;
		else
			tu->cursorCharOffset += offset;
	}

	tu->cursorBlinkTime = 0;
}

program_external note* GetCurrentNote() {
	return state.notes.selected;
}

program_external ui16 GetCharOffset(char* c) {
	Assert(TextIsBeingWritten() == true);
	return (ui16)((ui8*)c - (ui8*)GetTextStart(*state.textUpdate.textBody));
}

program_external char* FindChar(char c, ui16 pos, bool scanForward) {
	Assert(TextIsBeingWritten() == true);

	auto* tu = &state.textUpdate;
	if(scanForward == false && pos == 0)
		return Null;

	char* text = GetTextStart(*tu->textBody);
	ui32  start = scanForward == true ? pos : pos - 1;
	ui32  end = scanForward == true ? GetTextLength(*tu->textBody) : 0;
	FromTo(start, end) {
		if(text[it] == c)
			return text + it;
	}

	return Null;
}

program_external bool NoteIsBeingUpdated() {
	return TextIsBeingWritten() == true && state.notes.selected != Null && state.textUpdate.textBody->memory.memory == &state.notes.selected->text.memory.memory;
}

program_external void EndWriting() {
	ClearStruct(state.textUpdate);
}

program_external void InsertTextAtCursor(char c) {
	auto* tu = &state.textUpdate;
	Assert(tu->textBody != Null);
	InsertString(&c, 1, *tu->textBody, tu->cursorCharOffset);
	tu->cursorCharOffset += 1;
}

program_external bool TextIsBeingWritten() {
	return state.textUpdate.textBody != Null;
}

program_external f32 UI8ColourToF32(ui8 u) {
	return (f32)u / 255;
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

#include <windows.h>
#include <gl\gl.h>
program_external void DrawRectangle(f32 left, f32 bottom, f32 width, f32 height, ui8 r, ui8 g, ui8 b) {
	f32 rf = UI8ColourToF32(r);
	f32 gf = UI8ColourToF32(g);
	f32 bf = UI8ColourToF32(b);
	glBegin(GL_QUADS);
	glColor3f(rf, gf, bf);
	glVertex2f(left, bottom);
	glVertex2f(left + width, bottom);
	glVertex2f(left + width, bottom + height);
	glVertex2f(left, bottom + height);
	glEnd();
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