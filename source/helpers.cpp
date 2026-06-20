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

program_local void RenderTextLineHor(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x + height, y);
}

program_local void RenderTextLineVert(ui16 x, ui16 y, ui8 height) {
	glVertex2f(x, y);
	glVertex2f(x, y + height);
}

program_external void SetCursorPos(f32 x, f32 y) {
	state.textUpdate.cursorPos.x = x;
	state.textUpdate.cursorPos.y = y;
}

program_external text_body AllocateTextBody(f32 textHeight, bool allowSpecialChars) {
	text_body ret = {};
	ret.memory = AllocateStack();
	ret.specialCharsAllowed = allowSpecialChars;
	return ret;
}

program_external void BeginWriting(text_body& text, rectangle* containerBackground, f32 textHeight, bool leftAligned) {
	Assert(containerBackground != Null);
	state.textUpdate.textBody = &text;
	state.textUpdate.containerBackground = containerBackground;
	state.textUpdate.cursorOffset = text.memory.size;
	state.textUpdate.textHeight = textHeight;
	state.textUpdate.leftAligned = leftAligned;
}

program_external bool TextBodyIsValid(text_body& tb) {
	return IsValid(tb.memory);
}

program_external void FreeTextBody(text_body& tb) {
	if(TextBodyIsValid(tb) == true)
		FreeStack(tb.memory);
}

program_external bool NoteMemoryIsInUse(note* n) {
	Assert(n != Null);
	return n->background.width != 0 && n->background.height != 0;
}

program_external bool NoteHasTitle(note* n) {
	Assert(n != Null);
	return TextBodyIsValid(n->title);
}

program_external void DrawBorder(rectangle& r) {
	glLineWidth(2);
	glBegin(GL_LINES);
	glColor3f(0, 0, 0);
	
	glVertex2f(r.left, r.bottom);
	glVertex2f(r.left, r.bottom + r.height);
	
	glVertex2f(r.left, r.bottom + r.height);
	glVertex2f(r.left + r.width, r.bottom + r.height);
	
	glVertex2f(r.left + r.width, r.bottom + r.height);
	glVertex2f(r.left + r.width, r.bottom);
	
	glVertex2f(r.left + r.width, r.bottom);
	glVertex2f(r.left, r.bottom);
	glEnd();
	AssertOpenGL();	
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

program_external bool MouseLeftClickThisFrame() {
	return state.mouse.lastLeftDown == false && state.mouse.leftDown == true;
}

program_external bool MouseIsWithinToolbar() {
	return Overlap(state.mouse.pos.x, state.mouse.pos.y, UnpackDimensions(state.toolBar.background));
}

program_external bool TitleIsBeingUpdated() {
	return TextIsBeingWritten() == true && state.textUpdate.containerBackground == &state.titleBar.background;
}

program_external void MoveCursor(si8 offset) {
	Assert(TextIsBeingWritten() == true);
	
	auto* tu = &state.textUpdate;
	auto  textLength = GetTextLength(*tu->textBody);
	Assert(tu->cursorOffset <= textLength);
	
	if(offset < 0) {
		if(-offset >= tu->cursorOffset)
			tu->cursorOffset = 0;
		else
			tu->cursorOffset += offset;
	}
	else if(offset > 0) {
		if(tu->cursorOffset + offset >= textLength)
			tu->cursorOffset = textLength;
		else
			tu->cursorOffset += offset;
	}
}

program_external note* GetCurrentNote() {
	return state.notes.selected;
}

program_external ui16 GetCharOffset(char* c) {
	Assert(TextIsBeingWritten() == true);
	return (ui16)((ui8*)c - (ui8*)GetTextStart(*state.textUpdate.textBody));
}

program_external char* GetTextStart(text_body& tb) {
	Assert(TextBodyIsValid(tb) == true);
	return (char*)tb.memory.memory;
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

program_external void RemoveChar(text_body& tb, ui32 pos) {
	Assert(TextBodyIsValid(tb) == true);		
	if(pos < GetTextLength(tb))
		Remove(sizeof(char), pos, tb.memory);
}

program_external bool NoteIsBeingUpdated() {
	return TextIsBeingWritten() == true && state.notes.selected != Null && state.textUpdate.containerBackground == &state.notes.selected->background;
}

program_external void EndWriting() {
	ClearStruct(state.textUpdate);
}

program_external void AddText(char c) {
	auto* tu = &state.textUpdate;
	Assert(tu->textBody != Null);
	InsertText(&c, 1, *tu->textBody, tu->cursorOffset);
	tu->cursorOffset += 1;
}

program_external void InsertText(char* string, ui32 length, text_body& tb, ui32 pos) {
	Assert(string != Null);
	Assert(length > 0);
	Assert(TextBodyIsValid(tb) == true);
	if(pos <= tb.memory.size) {
		void* mem = Insert(length, pos, tb.memory);
		CopyMemory((void*)string, length, mem); 
	}
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

// @TODO - Export to API?
program_external void ResetProjectionMatrix() {
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

program_external ui32 GetTextLength(text_body& tb) {
	Assert(TextBodyIsValid(tb) == true);
	return tb.memory.size;
}

program_external void SetCanvasProjetionMatrix() {
	ResetProjectionMatrix();
	if(state.canvas.scale != 1.0f)
		glScalef(state.canvas.scale, state.canvas.scale, 1.0f);
	if(state.canvas.translation.x != 0 || state.canvas.translation.y != 0)
		glTranslatef(state.canvas.translation.x / state.canvas.scale, state.canvas.translation.y / state.canvas.scale, Null);
	AssertOpenGL();		
}

program_external vector GetTextRenderDimensions(char* text, ui32 length, f32 height) {
	Assert(text != Null);
	
	vector ret = { Null, height };
	if(length == 0)
		return ret;
	
	f32 xOffset = 0;
	ForAll(length) {
		if(text[it] == '\n') {
			xOffset = 0; 
			ret.y += height * 1.5f;
		}
		else {
			if(xOffset > 0)
				xOffset += height * 0.5f; // Space between glyphs
			xOffset += height;
			ret.x = GetMax(xOffset, ret.x);
		}
	}
	
	return ret;
}

program_external rectangle RenderText(char* text, ui32 length, f32 x, f32 y, f32 height, bool center) {
	Assert(text != Null);
	Assert(length > 0);
	
	rectangle ret = {};
	ret.left = x;
	ret.bottom = y;
	
	f32 xOffset = 0;
	if(center == true)
		xOffset = -GetTextRenderDimensions(text, length, height).x / 2;
	
	f32 nextX = x + xOffset;
	f32 nextY = y;
	glColor3f(1, 0, 0);
	glLineWidth(3);
	glBegin(GL_LINES);
	ForAll(length) {
		char c = text[it];
		switch(c) {
			case(' '): break;
			
			case('\n'): {
				nextY -= height * 1.5f; 
				ret.bottom = nextY;
				nextX = x; 
			} break;
			
			case('\b'): {
				RenderTextLineVert(nextX + height / 2, nextY + height / 4, height / 2);
				RenderTextLineHor(nextX + height / 4, nextY + height / 2, height / 2);
			} break;
			
			case('a'):
			case('A'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case('b'):
			case('B'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('c'):
			case ('C'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('d'):
			case ('D'): {
				RenderTextLineVert(nextX, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('e'):
			case ('E'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('f'):
			case ('F'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case ('g'):
			case ('G'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
				glVertex2f(nextX + height, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('h'):
			case ('H'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
			} break;
			
			case ('i'):
			case ('I'): {
				RenderTextLineVert(nextX + height / 2, nextY, height);
			} break;
			
			case ('j'):
			case ('J'): {
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
			} break;
			
			case ('k'):
			case ('K'): {
				RenderTextLineVert(nextX, nextY, height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('l'):
			case ('L'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;

			case ('m'):
			case ('M'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			case ('n'):
			case ('N'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
			} break;

			case ('o'):
			case ('O'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('p'):
			case ('P'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('q'):
			case ('Q'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height / 2, nextY, height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('r'):
			case ('R'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('s'):
			case ('S'): {
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY + height / 2, height);
				RenderTextLineHor(nextX, nextY, height);
				glVertex2f(nextX, nextY + height / 2);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
				glVertex2f(nextX + height, nextY + height / 2);
			} break;
			
			case ('t'):
			case ('T'): {
				RenderTextLineVert(nextX + height / 2, nextY, height);
				RenderTextLineHor(nextX, nextY + height, height);
			} break;
			
			case ('u'):
			case ('U'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				RenderTextLineHor(nextX, nextY, height);
			} break;
			
			case ('v'):
			case ('V'): {
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			case ('w'):
			case ('W'): {
				RenderTextLineVert(nextX, nextY, height);
				RenderTextLineVert(nextX + height, nextY, height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('x'):
			case ('X'): {
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height, nextY);
			} break;
			
			case ('y'):
			case ('Y'): {
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX + height, nextY + height);
				glVertex2f(nextX + height / 2, nextY + height / 2);
				glVertex2f(nextX, nextY + height);
				glVertex2f(nextX + height / 2, nextY);
				glVertex2f(nextX + height / 2, nextY + height / 2);
			} break;
			
			case ('z'):
			case ('Z'): {
				RenderTextLineHor(nextX, nextY + height, height);
				RenderTextLineHor(nextX, nextY, height);
				glVertex2f(nextX, nextY);
				glVertex2f(nextX + height, nextY + height);
			} break;
			
			default: break;
		}
		
		ret.width = GetMax(ret.width, nextX + height - ret.left);
			
		if(c != '\n')
			nextX += height * 1.5f;
	}
	glEnd();
	AssertOpenGL();
	
	ret.height = y + height - ret.bottom;
	Assert(ret.width != 0);
	Assert(ret.height != 0);
	
	return ret;
}